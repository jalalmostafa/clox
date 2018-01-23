#include "tokenizer.h"
#include "ds/list.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token* token(TokenType type, char* literal, int line, int column)
{
    int length = 0;
    Token* token = (Token*)alloc(sizeof(Token));
    token->type = type;
    if (literal == NULL) {
        token->literal = NULL;
    } else {
        length = strlen(literal) + 1;
        token->literal = (char*)alloc(length);
        strncpy(token->literal, literal, length);
    }
    token->line = line;
    token->column = column;
    return token;
}

static Token* token_simple(TokenType type, int line, int column)
{
    return token(type, NULL, line, column);
}

static void token_destroy(void* data)
{
    Token* token = (Token*)data;
    fr(token->literal);
    fr(token);
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

Tokenization* toknzr(const char* code)
{
    char* literal = NULL;
    TokenType type = EOF;
    int length = strlen(code), current = 0, line = 1;
    Tokenization* toknz = (Tokenization*)alloc(sizeof(Tokenization));
    toknz->values = list();
    toknz->lines = 0;
    while (!IS_AT_END(current, length)) {
        char c = code[current];
        switch (c) {
        case '(':
            list_push(toknz->values, token_simple(LEFT_PAREN, line, current));
            break;
        case ')':
            list_push(toknz->values, token_simple(RIGHT_PAREN, line, current));
            break;
        case '{':
            list_push(toknz->values, token_simple(LEFT_BRACE, line, current));
            break;
        case '}':
            list_push(toknz->values, token_simple(RIGHT_BRACE, line, current));
            break;
        case ',':
            list_push(toknz->values, token_simple(COMMA, line, current));
            break;
        case '.':
            list_push(toknz->values, token_simple(DOT, line, current));
            break;
        case '-':
            list_push(toknz->values, token_simple(MINUS, line, current));
            break;
        case '+':
            list_push(toknz->values, token_simple(PLUS, line, current));
            break;
        case ';':
            list_push(toknz->values, token_simple(SEMICOLON, line, current));
            break;
        case '*':
            list_push(toknz->values, token_simple(STAR, line, current));
            break;
        case '!':
            type = match_next(code, '=', length, &current) ? BANG_EQUAL : BANG;
            list_push(toknz->values, token_simple(type, line, current));
            break;
        case '=':
            type = match_next(code, '=', length, &current) ? EQUAL_EQUAL : EQUAL;
            list_push(toknz->values, token_simple(type, line, current));
            break;
        case '>':
            type = match_next(code, '=', length, &current) ? GREATER_EQUAL : GREATER;
            list_push(toknz->values, token_simple(type, line, current));
            break;
        case '<':
            type = match_next(code, '=', length, &current) ? LESS_EQUAL : LESS;
            list_push(toknz->values, token_simple(type, line, current));
            break;
        case '/':
            type = match_next(code, '/', length, &current) ? EOF : SLASH;
            if (type == EOF) {
                do {
                    current++;
                } while (current != length && code[current] != '\n');
            } else {
                list_push(toknz->values, token_simple(SLASH, line, current));
            }
            break;
        case '"':
            literal = read_between(code, length, &current, &line, '"');
            if (literal != NULL) {
                list_push(toknz->values, token(STRING, literal, line, current));
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
                list_push(toknz->values, token(NUMBER, literal, line, current));
            } else {
                toknzr_error(line, current, c);
            }
            break;
        }
#ifdef DEBUG
        if (literal != NULL) {
            printf("Literal: %s\n", literal);
        }
#endif
        fr(literal);
        literal = NULL;
        current++;
    }
    toknz->lines = line;
    list_push(toknz->values, token_simple(EOF, line, current));
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
