#include "vm/compiler.h"
#include "ds/list.h"
#include "tokenizer.h"
#include "vm/common.h"
#ifdef DEBUG_PRINT_CODE
#include "vm/debug.h"
#endif
#include <stdio.h>
#include <string.h>

static void number();
static void binary();
static void unary();
static void grouping();
static void expression();

static void advance();
static void error(const char* message);
static void error_at(Node* node, const char* message);

typedef struct vm_parser {
    Node* current;
    Node* previous;
    Node* last;
    int panicMode;
    int hadError;
} VmParser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

ParseRule rules[] = {
    { grouping, NULL, PREC_CALL }, // TOKEN_LEFT_PAREN
    { NULL, NULL, PREC_NONE }, // TOKEN_RIGHT_PAREN
    { NULL, NULL, PREC_NONE }, // TOKEN_LEFT_BRACE
    { NULL, NULL, PREC_NONE }, // TOKEN_RIGHT_BRACE
    { NULL, NULL, PREC_NONE }, // TOKEN_COMMA
    { NULL, NULL, PREC_CALL }, // TOKEN_DOT
    { unary, binary, PREC_TERM }, // TOKEN_MINUS
    { NULL, binary, PREC_TERM }, // TOKEN_PLUS
    { NULL, NULL, PREC_NONE }, // TOKEN_SEMICOLON
    { NULL, binary, PREC_FACTOR }, // TOKEN_SLASH
    { NULL, binary, PREC_FACTOR }, // TOKEN_STAR
    { NULL, NULL, PREC_NONE }, // TOKEN_BANG
    { NULL, NULL, PREC_EQUALITY }, // TOKEN_BANG_EQUAL
    { NULL, NULL, PREC_NONE }, // TOKEN_EQUAL
    { NULL, NULL, PREC_EQUALITY }, // TOKEN_EQUAL_EQUAL
    { NULL, NULL, PREC_COMPARISON }, // TOKEN_GREATER
    { NULL, NULL, PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
    { NULL, NULL, PREC_COMPARISON }, // TOKEN_LESS
    { NULL, NULL, PREC_COMPARISON }, // TOKEN_LESS_EQUAL
    { NULL, NULL, PREC_NONE }, // TOKEN_IDENTIFIER
    { NULL, NULL, PREC_NONE }, // TOKEN_STRING
    { number, NULL, PREC_NONE }, // TOKEN_NUMBER
    { NULL, NULL, PREC_AND }, // TOKEN_AND
    { NULL, NULL, PREC_NONE }, // TOKEN_CLASS
    { NULL, NULL, PREC_NONE }, // TOKEN_ELSE
    { NULL, NULL, PREC_NONE }, // TOKEN_FALSE
    { NULL, NULL, PREC_NONE }, // TOKEN_FOR
    { NULL, NULL, PREC_NONE }, // TOKEN_FUN
    { NULL, NULL, PREC_NONE }, // TOKEN_IF
    { NULL, NULL, PREC_NONE }, // TOKEN_NIL
    { NULL, NULL, PREC_OR }, // TOKEN_OR
    { NULL, NULL, PREC_NONE }, // TOKEN_PRINT
    { NULL, NULL, PREC_NONE }, // TOKEN_RETURN
    { NULL, NULL, PREC_NONE }, // TOKEN_SUPER
    { NULL, NULL, PREC_NONE }, // TOKEN_THIS
    { NULL, NULL, PREC_NONE }, // TOKEN_TRUE
    { NULL, NULL, PREC_NONE }, // TOKEN_VAR
    { NULL, NULL, PREC_NONE }, // TOKEN_WHILE
    { NULL, NULL, PREC_NONE }, // TOKEN_ERROR
    { NULL, NULL, PREC_NONE }, // TOKEN_ENDOFFILE
};

static Chunk* compilingChunk;

static VmParser parser;

static ParseRule* parse_rule(TokenType type)
{
    return &rules[type];
}

static void prec_parse(Precedence prec)
{
    ParseFn prefixRule, infixRule;
    Token* token = NULL;

    advance();
    token = (Token*)parser.previous->data;
    prefixRule = parse_rule(token->type)->prefix;

    if (prefixRule == NULL) {
        error("Expect Expression.");
        return;
    }

    prefixRule();

    while (prec <= parse_rule(((Token*)parser.current->data)->type)->precedence) {
        advance();
        infixRule = parse_rule(((Token*)parser.previous->data)->type)->infix;
        infixRule();
    }
}

static void error_at(Node* node, const char* message)
{
    Token* token = (Token*)node->data;
    if (parser.panicMode) {
        return;
    }

    parser.panicMode = 1;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_ENDOFFILE) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%s'", token->lexeme);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = 1;
}

static void error(const char* message)
{
    error_at(parser.previous, message);
}

static void error_at_current(const char* message)
{
    error_at(parser.current, message);
}

static void advance()
{
    Node* node = NULL;
    Token* token = NULL;

    parser.previous = parser.current;

    for (node = parser.current; node != parser.last; node = node->next) {
        parser.current = node;
        token = (Token*)node->data;
        if (token->type != TOKEN_ERROR)
            break;

        error_at_current(token->lexeme);
    }
}

static void consume(TokenType type, const char* message)
{
    Token* token = (Token*)parser.current->data;
    if (token->type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

static Chunk* current_chunk()
{
    return compilingChunk;
}

static void foreach_token(List* toknz, void* toknObj)
{
    int line = -1;
    Token* token = (Token*)toknObj;

    if (token->line != line) {
        printf("%4d ", token->line);
        line = token->line;
    } else {
        printf("   | ");
    }
    printf("%2d '%s'\n", token->type, token->lexeme);
}

static Byte make_constant(Value value)
{
    int constant = chunk_constants_add(current_chunk(), value);
    if (constant > BYTE_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (Byte)constant;
}

static void emit_byte(Byte byte)
{
    Token* token = (Token*)parser.previous->data;
    chunk_write(current_chunk(), byte, token->line);
}

static void emit_bytes(Byte byte1, Byte byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_constant(Value value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void emit_return()
{
    emit_byte(OP_RETURN);
}

static void compiler_end()
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        chunk_disassemble(current_chunk(), "code");
    }
#endif
}

static void expression()
{
    prec_parse(PREC_ASSIGNMENT);
}

static void number()
{
    Token* token = (Token*)parser.previous->data;
    double value = strtod(token->lexeme, NULL);
    emit_constant(value);
}

static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary()
{
    Token* token = (Token*)parser.previous->data;
    TokenType operatorType = token->type;

    prec_parse(PREC_UNARY);

    switch (operatorType) {
    case TOKEN_MINUS:
        emit_byte(OP_NEGATE);
        break;
    default:
        return;
    }
}

static void binary()
{
    Token* token = (Token*)parser.previous->data;
    TokenType operatorType = token->type;

    ParseRule* rule = parse_rule(operatorType);
    prec_parse((Precedence)(rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_PLUS:
        emit_byte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emit_byte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emit_byte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emit_byte(OP_DIVIDE);
        break;
    default:
        return;
    }
}

int compile(const char* code, Chunk* chunk)
{
    Tokenization toknz = toknzr(code, 0);
    list_foreach(toknz.values, foreach_token);

    compilingChunk = chunk;
    parser.last = toknz.values->last;
    parser.current = toknz.values->head;
    parser.previous = NULL;
    parser.hadError = 0;
    parser.panicMode = 0;

    advance();

    compiler_end();
    toknzr_destroy(toknz);
    return !parser.hadError;
}
