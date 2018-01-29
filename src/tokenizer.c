#include "tokenizer.h"
#include "ds/list.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token* token(TokenType type, char* literal, int line, int column, char* lexeme)
{
    int length = 0;
    Token* tokn = (Token*)alloc(sizeof(Token));
    tokn->type = type;
    if (literal == NULL) {
        tokn->literal = NULL;
    } else {
        length = strlen(literal) + 1;
        tokn->literal = (char*)alloc(length);
        strncpy(tokn->literal, literal, length);
    }
    if (lexeme == NULL) {
        tokn->lexeme = NULL;
    } else {
        length = strlen(lexeme) + 1;
        tokn->lexeme = (char*)alloc(length);
        strncpy(tokn->lexeme, lexeme, length);
    }
    tokn->line = line;
    tokn->column = column;
    return tokn;
}

static Token* token_simple(TokenType type, int line, int column, char* lexeme)
{
    return token(type, NULL, line, column, lexeme);
}

static void token_destroy(void* data)
{
    Token* tokn = (Token*)data;
    fr(tokn->literal);
    fr(tokn);
}

static void toknzr_error(int line, int column, char c)
{
    char buf[LINEBUFSIZE];
    memset(buf, 0, LINEBUFSIZE);
    snprintf(buf, LINEBUFSIZE, "Syntax Error at (%d, %d): %c is unexpected", line, column + 1, c);
    puts(buf);
}

static int match_next(const char* code, char next, int length, int* current)
{
    if (current == NULL || (*current) > length) {
        return 0;
    }

    if (code[(*current) + 1] != next) {
        return 0;
    }

    (*current)++;
    return 1;
}

static char* read_between(const char* code, int codeLength, int* current, int* line, char to)
{
    char* literal = NULL;
    int start = *current, length = 0;
    do {
        if (code[*current] == '\n') {
            *line++;
        }
        (*current)++;
    } while (code[*current] != to && !IS_AT_END(*current, codeLength));
    if (IS_AT_END(*current, codeLength)) {
        toknzr_error(*line, *current, code[*current]);
        return NULL;
    }
    length = *current - start;
    literal = (char*)alloc(length);
    memcpy(literal, &(code[start + 1]), length);
    literal[length - 1] = '\0';
    return literal;
}
#ifdef DEBUG
static char* to_type_string(TokenType type)
{
    switch (type) {
    case LEFT_PAREN:
        return "LEFT_PAREN";
    case RIGHT_PAREN:
        return "RIGHT_PAREN";
    case LEFT_BRACE:
        return "LEFT_BRACE";
    case RIGHT_BRACE:
        return "RIGHT_BRACE";
    case COMMA:
        return "COMMA";
    case DOT:
        return "DOT";
    case MINUS:
        return "MINUS";
    case PLUS:
        return "PLUS";
    case SEMICOLON:
        return "SEMICOLON";
    case SLASH:
        return "SLASH";
    case STAR:
        return "STAR";
    case BANG:
        return "BANG";
    case BANG_EQUAL:
        return "BANG_EQUAL";
    case EQUAL:
        return "EQUAL";
    case EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case GREATER:
        return "GREATER";
    case GREATER_EQUAL:
        return "GREATER_EQUAL";
    case LESS:
        return "LESS";
    case LESS_EQUAL:
        return "LESS_EQUAL";
    case IDENTIFIER:
        return "IDENTIFIER";
    case STRING:
        return "STRING";
    case NUMBER:
        return "NUMBER";
    case AND:
        return AND_KEY;
    case CLASS:
        return CLASS_KEY;
    case ELSE:
        return ELSE_KEY;
    case FALSE:
        return FALSE_KEY;
    case FUN:
        return FUN_KEY;
    case FOR:
        return FOR_KEY;
    case IF:
        return IF_KEY;
    case NIL:
        return NIL_KEY;
    case OR:
        return OR_KEY;
    case PRINT:
        return PRINT_KEY;
    case RETURN:
        return RETURN_KEY;
    case SUPER:
        return SUPER_KEY;
    case THIS:
        return THIS_KEY;
    case TRUE:
        return TRUE_KEY;
    case VAR:
        return VAR_KEY;
    case WHILE:
        return WHILE_KEY;
    case EOF:
        return "EOF";
    default:
        return "Unknown";
    }
}
#endif

static char* read_number(const char* code, int codeLength, int* current)
{
    char* literal = NULL;
    int start = *current, length = 0;
    do {
        (*current)++;
    } while (!IS_AT_END(*current, codeLength) && isdigit(code[*current]));
    if (code[*current] == '.' && isdigit(code[(*current) + 1])) {
        do {
            (*current)++;
        } while (!IS_AT_END(*current, codeLength) && isdigit(code[*current]));
    }
    length = *current - start + 1;
    literal = (char*)alloc(length);
    memcpy(literal, &(code[start]), length);
    literal[length - 1] = '\0';
    return literal;
}

static char* read_other(const char* code, int codeLength, int* current)
{
    char* literal = NULL;
    int start = *current, length = 0;
    do {
        (*current)++;
    } while (!IS_AT_END(*current, codeLength) && IS_ALPHA_NUMERIC(code[*current]));
    length = *current - start + 1;
    literal = (char*)alloc(length);
    memcpy(literal, &(code[start]), length);
    literal[length - 1] = '\0';
    return literal;
}

Tokenization* toknzr(const char* code)
{
    char* literal = NULL;
    Token* tokn = NULL;
    TokenType type = EOF;
    int length = strlen(code), current = 0, line = 1;
    Tokenization* toknz = (Tokenization*)alloc(sizeof(Tokenization));
    toknz->values = list();
    toknz->lines = 0;
    while (!IS_AT_END(current, length)) {
        char c = code[current];
        switch (c) {
        case '(':
            tokn = token_simple(LEFT_PAREN, line, current, "(");
            break;
        case ')':
            tokn = token_simple(RIGHT_PAREN, line, current, ")");
            break;
        case '{':
            tokn = token_simple(LEFT_BRACE, line, current, "{");
            break;
        case '}':
            tokn = token_simple(RIGHT_BRACE, line, current, "}");
            break;
        case ',':
            tokn = token_simple(COMMA, line, current, ",");
            break;
        case '.':
            tokn = token_simple(DOT, line, current, ".");
            break;
        case '-':
            tokn = token_simple(MINUS, line, current, "-");
            break;
        case '+':
            tokn = token_simple(PLUS, line, current, "+");
            break;
        case ';':
            tokn = token_simple(SEMICOLON, line, current, ";");
            break;
        case '*':
            tokn = token_simple(STAR, line, current, "*");
            break;
        case '!':
            type = match_next(code, '=', length, &current) ? BANG_EQUAL : BANG;
            tokn = token_simple(type, line, current, "!");
            break;
        case '=':
            type = match_next(code, '=', length, &current) ? EQUAL_EQUAL : EQUAL;
            tokn = token_simple(type, line, current, "=");
            break;
        case '>':
            type = match_next(code, '=', length, &current) ? GREATER_EQUAL : GREATER;
            tokn = token_simple(type, line, current, ">");
            break;
        case '<':
            type = match_next(code, '=', length, &current) ? LESS_EQUAL : LESS;
            tokn = token_simple(type, line, current, "<");
            break;
        case '/':
            type = match_next(code, '/', length, &current) ? EOF : SLASH;
            if (type == EOF) {
                do {
                    current++;
                } while (current != length && code[current] != '\n');
            } else {
                tokn = token_simple(SLASH, line, current, "/");
            }
            break;
        case '"':
            literal = read_between(code, length, &current, &line, '"');
            if (literal != NULL) {
                tokn = token(STRING, literal, line, current, "\"");
            }
            break;
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line++;
            break;
        default:
            if (isdigit(c)) {
                literal = read_number(code, length, &current);
                tokn = token(NUMBER, literal, line, current, literal);
            } else if (isalpha(c)) {
                literal = read_other(code, length, &current);
                if (strcmp(literal, AND_KEY) == 0) {
                    tokn = token_simple(AND, line, current, AND_KEY);
                } else if (strcmp(literal, CLASS_KEY) == 0) {
                    tokn = token_simple(CLASS, line, current, CLASS_KEY);
                } else if (strcmp(literal, ELSE_KEY) == 0) {
                    tokn = token_simple(ELSE, line, current, ELSE_KEY);
                } else if (strcmp(literal, FALSE_KEY) == 0) {
                    tokn = token_simple(FALSE, line, current, FALSE_KEY);
                } else if (strcmp(literal, FUN_KEY) == 0) {
                    tokn = token_simple(FUN, line, current, FUN_KEY);
                } else if (strcmp(literal, FOR_KEY) == 0) {
                    tokn = token_simple(FOR, line, current, FOR_KEY);
                } else if (strcmp(literal, IF_KEY) == 0) {
                    tokn = token_simple(IF, line, current, IF_KEY);
                } else if (strcmp(literal, NIL_KEY) == 0) {
                    tokn = token_simple(NIL, line, current, NIL_KEY);
                } else if (strcmp(literal, OR_KEY) == 0) {
                    tokn = token_simple(OR, line, current, OR_KEY);
                } else if (strcmp(literal, PRINT_KEY) == 0) {
                    tokn = token_simple(PRINT, line, current, PRINT_KEY);
                } else if (strcmp(literal, RETURN_KEY) == 0) {
                    tokn = token_simple(RETURN, line, current, RETURN_KEY);
                } else if (strcmp(literal, SUPER_KEY) == 0) {
                    tokn = token_simple(SUPER, line, current, SUPER);
                } else if (strcmp(literal, THIS_KEY) == 0) {
                    tokn = token_simple(THIS, line, current, THIS_KEY);
                } else if (strcmp(literal, TRUE_KEY) == 0) {
                    tokn = token_simple(TRUE, line, current, TRUE_KEY);
                } else if (strcmp(literal, VAR_KEY) == 0) {
                    tokn = token_simple(VAR, line, current, VAR_KEY);
                } else if (strcmp(literal, WHILE_KEY) == 0) {
                    tokn = token_simple(WHILE, line, current, WHILE_KEY);
                } else {
                    tokn = token_simple(IDENTIFIER, line, current, literal);
                }
            } else {
                toknzr_error(line, current, c);
            }
            break;
        }
#ifdef DEBUG
        if (literal != NULL) {
            printf("Literal: %s\n", literal);
        }
        printf("Type: %s\n", to_type_string(tokn->type));
#endif
        fr(literal);
        literal = NULL;
        current++;
        list_push(toknz->values, tokn);
    }
    toknz->lines = line;
    list_push(toknz->values, token_simple(EOF, line, current, "EOF"));
    return toknz;
}

void toknzr_destroy(Tokenization* toknz)
{
    if (toknz == NULL) {
        return;
    }
    list_foreach(toknz->values, token_destroy);
    list_destroy(toknz->values);
    fr(toknz);
}
