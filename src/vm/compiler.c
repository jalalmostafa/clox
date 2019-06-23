#include "vm/compiler.h"
#include "ds/list.h"
#include "tokenizer.h"
#include "vm/common.h"
#include "vm/table.h"
#include "vm/vm.h"
#ifdef DEBUG_PRINT_CODE
#include "vm/debug.h"
#endif
#include <stdio.h>
#include <string.h>

static void variable(int canAssign);
static void literal(int canAssign);
static void string(int canAssign);
static void number(int canAssign);
static void binary(int canAssign);
static void unary(int canAssign);
static void grouping(int canAssign);
static void _and(int canAssign);
static void _or(int canAssign);
static void expression();

static void var_declaration();
static void declaration();
static void statement();
static void print_statement();
static void expression_statement();

static int check(TokenType type);
static int match(TokenType type);
static void advance();
static void error(const char* message);
static void error_at(Node* node, const char* message);

static int identifier_equal(Token* a, Token* b);
static Byte identifier_constant(Node* node);
static Byte variable_parse(const char* message);
static void variable_define(Byte id);
static void named_variable(Node* node, int canAssign);

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

typedef void (*ParseFn)(int canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct Compiler {
    Local locals[BYTE_COUNT];
    int localCount;
    int scopeDepth;
} VmCompiler;

static int variable_local_resolve(VmCompiler* compiler, Token* name);

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
    { unary, NULL, PREC_NONE }, // TOKEN_BANG
    { NULL, binary, PREC_EQUALITY }, // TOKEN_BANG_EQUAL
    { NULL, NULL, PREC_NONE }, // TOKEN_EQUAL
    { NULL, binary, PREC_EQUALITY }, // TOKEN_EQUAL_EQUAL
    { NULL, binary, PREC_COMPARISON }, // TOKEN_GREATER
    { NULL, binary, PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
    { NULL, binary, PREC_COMPARISON }, // TOKEN_LESS
    { NULL, binary, PREC_COMPARISON }, // TOKEN_LESS_EQUAL
    { variable, NULL, PREC_NONE }, // TOKEN_IDENTIFIER
    { string, NULL, PREC_NONE }, // TOKEN_STRING
    { number, NULL, PREC_NONE }, // TOKEN_NUMBER
    { NULL, _and, PREC_AND }, // TOKEN_AND
    { NULL, NULL, PREC_NONE }, // TOKEN_CLASS
    { NULL, NULL, PREC_NONE }, // TOKEN_ELSE
    { literal, NULL, PREC_NONE }, // TOKEN_FALSE
    { NULL, NULL, PREC_NONE }, // TOKEN_FOR
    { NULL, NULL, PREC_NONE }, // TOKEN_FUN
    { NULL, NULL, PREC_NONE }, // TOKEN_IF
    { literal, NULL, PREC_NONE }, // TOKEN_NIL
    { NULL, _or, PREC_OR }, // TOKEN_OR
    { NULL, NULL, PREC_NONE }, // TOKEN_PRINT
    { NULL, NULL, PREC_NONE }, // TOKEN_RETURN
    { NULL, NULL, PREC_NONE }, // TOKEN_SUPER
    { NULL, NULL, PREC_NONE }, // TOKEN_THIS
    { literal, NULL, PREC_NONE }, // TOKEN_TRUE
    { NULL, NULL, PREC_NONE }, // TOKEN_VAR
    { NULL, NULL, PREC_NONE }, // TOKEN_WHILE
    { NULL, NULL, PREC_NONE }, // TOKEN_ERROR
    { NULL, NULL, PREC_NONE }, // TOKEN_ENDOFFILE
};

static Chunk* compilingChunk;

static VmParser parser;

static VmCompiler* currentCompiler = NULL;

static ParseRule* parse_rule(TokenType type)
{
    return &rules[type];
}

static void prec_parse(Precedence prec)
{
    ParseFn prefixRule, infixRule;
    Token* token = NULL;
    int canAssign = 0;

    advance();
    token = (Token*)parser.previous->data;
    prefixRule = parse_rule(token->type)->prefix;

    if (prefixRule == NULL) {
        error("Expect Expression.");
        return;
    }

    canAssign = prec <= PREC_ASSIGNMENT;

    prefixRule(canAssign);

    while (prec <= parse_rule(((Token*)parser.current->data)->type)->precedence) {
        advance();
        infixRule = parse_rule(((Token*)parser.previous->data)->type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
        expression();
    }
}

static void synchronize()
{
    TokenType currentType = ((Token*)parser.current->data)->type;
    TokenType prevType = ((Token*)parser.previous->data)->type;
    parser.panicMode = 0;

    while (currentType != TOKEN_ENDOFFILE) {
        if (prevType == TOKEN_SEMICOLON)
            return;

        switch (currentType) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:
            // Do nothing.
            ;
        }

        advance();
    }
}

static int check(TokenType type)
{
    Token* token = (Token*)parser.current->data;
    return token->type == type;
}

static int match(TokenType type)
{
    if (!check(type)) {
        return 0;
    }

    advance();
    return 1;
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

    for (node = parser.current; node != parser.last;) {
        parser.current = node = node->next;
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

static int emit_jump(Byte instruction)
{
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);
    return current_chunk()->count - 2;
}

static void patch_jump(int offset)
{
    int jump = current_chunk()->count - offset - 2;

    if (jump > BYTE_MAX) {
        error("Too much code to jump over.");
    }

    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static void emit_loop(int loopStart)
{
    int offset = 0;
    emit_byte(OP_LOOP);

    offset = current_chunk()->count - loopStart + 2;

    if (offset > SHORT_MAX) {
        error("Loop body too large");
    }

    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

static void compiler_init(VmCompiler* compiler)
{
    memset(compiler, 0, sizeof(VmCompiler));
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    currentCompiler = compiler;
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

static void literal(int canAssign)
{
    Token* token = (Token*)parser.previous->data;
    switch (token->type) {
    case TOKEN_FALSE:
        emit_byte(OP_FALSE);
        break;
    case TOKEN_TRUE:
        emit_byte(OP_TRUE);
        break;
    case TOKEN_NIL:
        emit_byte(OP_NIL);
        break;
    default:
        return;
    }
}

static VmObject* new_vmobject(size_t size, VmObjectType type)
{
    VmObject* object = (VmObject*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

#define ALLOC_OBJECT(type, objectType) ((type*)new_vmobject(sizeof(type), (objectType)))

static Hash hash_string(const char* string, int length)
{
    unsigned int hash = 2166136261u;
    int i;

    for (i = 0; i < length; i++) {
        hash ^= string[i];
        hash *= 16777619;
    }

    return hash;
}

static VmString* new_vmstring(char* chars, int length, Hash hash)
{
    VmString* string = ALLOC_OBJECT(VmString, OBJECT_STRING);
    string->chars = chars;
    string->length = length;
    string->hash = hash;
    table_set(&vm.strings, string, nil_val());
    return string;
}

VmString* vmstring_copy(const char* chars, int length)
{
    char* heapChars = NULL;
    Hash hash = hash_string(chars, length);
    VmString* interned = table_find_string(&vm.strings, chars, length, hash);

    if (interned != NULL) {
        return interned;
    }

    heapChars = (char*)alloc(length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = 0;
    return new_vmstring(heapChars, length, hash);
}

VmString* vmstring_take(char* chars, int length)
{
    Hash hash = hash_string(chars, length);
    VmString* interned = table_find_string(&vm.strings, chars, length, hash);

    if (interned != NULL) {
        return interned;
    }

    return new_vmstring(chars, length, hash);
}

static void variable(int canAssign)
{
    named_variable(parser.previous, canAssign);
}

static void named_variable(Node* node, int canAssign)
{
    Byte getOp, setOp;
    Token* name = (Token*)node->data;
    int arg = variable_local_resolve(currentCompiler, name);

    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        arg = identifier_constant(node);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(setOp, (Byte)arg);
    } else {
        emit_bytes(getOp, (Byte)arg);
    }
}

static void string(int canAssign)
{
    Token* token = (Token*)parser.previous->data;
    VmString* string = vmstring_copy(token->literal, strlen(token->literal));
    Value stringValue = object_val((VmObject*)string);
    emit_constant(stringValue);
}

static void number(int canAssign)
{
    Token* token = (Token*)parser.previous->data;
    double value = strtod(token->lexeme, NULL);
    emit_constant(number_val(value));
}

static void grouping(int canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(int canAssign)
{
    Token* token = (Token*)parser.previous->data;
    TokenType operatorType = token->type;

    prec_parse(PREC_UNARY);

    switch (operatorType) {
    case TOKEN_MINUS:
        emit_byte(OP_NEGATE);
        break;
    case TOKEN_BANG:
        emit_byte(OP_NOT);
        break;
    default:
        return;
    }
}

static void binary(int canAssign)
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
    case TOKEN_BANG_EQUAL:
        emit_bytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        emit_byte(OP_EQUAL);
        break;
    case TOKEN_GREATER_EQUAL:
        emit_bytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_GREATER:
        emit_byte(OP_GREATER);
        break;
    case TOKEN_LESS:
        emit_byte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emit_bytes(OP_GREATER, OP_NOT);
        break;
    default:
        return;
    }
}

static void _and(int canAssign)
{
    int endJump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    prec_parse(PREC_AND);

    patch_jump(endJump);
}

static void _or(int canAssign)
{
    int elseJump = emit_jump(OP_JUMP_IF_FALSE);
    int endJump = emit_jump(OP_JUMP);

    patch_jump(elseJump);
    emit_byte(OP_POP);

    prec_parse(PREC_OR);
    patch_jump(endJump);
}

static void print_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(OP_PRINT);
}

static void expression_statement()
{
    expression();
    emit_byte(OP_POP);
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
}

static void while_statement()
{
    int loopStart = current_chunk()->count;
    int exitJump = 0;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    exitJump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    statement();

    emit_loop(loopStart);

    patch_jump(exitJump);
    emit_byte(OP_POP);
}

static void scope_begin()
{
    currentCompiler->scopeDepth++;
}

static void scope_end()
{
    currentCompiler->scopeDepth--;

    while (currentCompiler->localCount > 0
        && currentCompiler->locals[currentCompiler->localCount - 1].depth > currentCompiler->scopeDepth) {
        emit_byte(OP_POP);
        currentCompiler->localCount--;
    }
}

static void block_statement()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_ENDOFFILE)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void if_statement()
{
    int thenJump = 0, elseJump = 0;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    thenJump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    elseJump = emit_jump(OP_JUMP);
    patch_jump(thenJump);
    emit_byte(OP_POP);

    if (match(TOKEN_ELSE)) {
        statement();
    }
    patch_jump(elseJump);
}

static void for_statement()
{
    int loopStart = 0, exitJump = -1, bodyJump = 0, incrementStart = 0;
    scope_begin();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    if (match(TOKEN_VAR)) {
        var_declaration();
    } else if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else {
        expression_statement();
    }

    loopStart = current_chunk()->count;

    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        bodyJump = emit_jump(OP_JUMP);

        incrementStart = current_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(loopStart);
        loopStart = incrementStart;
        patch_jump(bodyJump);
    }

    statement();

    emit_loop(loopStart);

    if (exitJump != -1) {
        patch_jump(exitJump);
        emit_byte(OP_POP);
    }

    scope_end();
}

static void statement()
{
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_FOR)) {
        for_statement();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        scope_begin();
        block_statement();
        scope_end();
    } else {
        expression_statement();
    }
}

static Byte identifier_constant(Node* node)
{
    Token* token = (Token*)node->data;
    VmString* identifier = vmstring_copy(token->lexeme, strlen(token->lexeme));
    return make_constant(object_val((VmObject*)identifier));
}

static void variable_initialize()
{
    if (currentCompiler->scopeDepth == 0) {
        return;
    }

    currentCompiler->locals[currentCompiler->localCount - 1].depth = currentCompiler->scopeDepth;
}

static void variable_define(Byte variableId)
{
    if (currentCompiler->scopeDepth > 0) {
        variable_initialize();
        return;
    }

    emit_bytes(OP_DEFINE_GLOBAL, variableId);
}

static int identifier_equal(Token* a, Token* b)
{
    int aLength = a == NULL || a->lexeme == NULL ? 0 : strlen(a->lexeme),
        bLength = b == NULL || b->lexeme == NULL ? 0 : strlen(b->lexeme);
    if (aLength != bLength) {
        return 0;
    }

    return memcmp(a->lexeme, b->lexeme, aLength) == 0;
}

static int variable_local_resolve(VmCompiler* compiler, Token* name)
{
    Local* local = NULL;
    int i = 0;
    for (i = compiler->localCount - 1; i >= 0; i--) {
        local = &compiler->locals[i];
        if (identifier_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Cannot read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void variable_local_add(Token name)
{
    Local* local = NULL;

    if (currentCompiler->localCount == BYTE_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    local = &currentCompiler->locals[currentCompiler->localCount++];
    local->name = name;
    local->depth = -1;
}

static void variable_declare()
{
    Local* local = NULL;
    Token* name = NULL;
    int i = 0;

    if (currentCompiler->scopeDepth == 0) {
        return;
    }

    name = (Token*)parser.previous->data;
    for (i = currentCompiler->localCount - 1; i >= 0; i--) {
        local = &currentCompiler->locals[i];

        if (local->depth != -1 && local->depth < currentCompiler->scopeDepth) {
            break;
        }

        if (identifier_equal(name, &local->name)) {
            error("Variable with this name already declared in this scope.");
        }
    }
    variable_local_add(*name);
}

static Byte variable_parse(const char* message)
{
    consume(TOKEN_IDENTIFIER, message);

    variable_declare();
    if (currentCompiler->scopeDepth > 0) {
        return 0;
    }

    return identifier_constant(parser.previous);
}

static void var_declaration()
{
    Byte global = variable_parse("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    variable_define(global);
}

static void declaration()
{
    if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

int compile(const char* code, Chunk* chunk)
{
    VmCompiler compiler;
    Tokenization toknz = toknzr(code, 0);
#ifdef DEBUG_PRINT_CODE
    list_foreach(toknz.values, foreach_token);
#endif
    compiler_init(&compiler);

    compilingChunk = chunk;
    parser.last = toknz.values->last;
    parser.current = toknz.values->head;
    parser.previous = NULL;
    parser.hadError = 0;
    parser.panicMode = 0;

    while (!match(TOKEN_ENDOFFILE)) {
        declaration();
    }

    compiler_end();
    toknzr_destroy(toknz);
    return !parser.hadError;
}
