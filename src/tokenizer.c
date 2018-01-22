#include "tokenizer.h"
#include "ds/list.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token* token(TokenType type, char* luxeme, char* literal, int line, int column)
{
    int len = strlen(luxeme) + 1;
    Token* token = (Token*)alloc(sizeof(Token));
    token->type = type;
    token->luxeme = (char*)alloc(len);
    strncpy(token->luxeme, luxeme, len);
    if (literal == NULL) {
        token->literal = NULL;
    } else {
        len = strlen(literal) + 1;
        token->literal = (char*)alloc(len);
        strncpy(token->literal, literal, len);
    }
    token->line = line;
    token->column = column;
    return token;
}

static Token* token_simple(TokenType type, char* luxeme, int line, int column)
{
    return token(type, luxeme, NULL, line, column);
}

static void token_destroy(void* data)
{
    Token* token = (Token*)data;
    fr(token->literal);
    fr(token->luxeme);
    fr(token);
}

static void toknzr_error(int line, int column, char* luxeme)
{
    char buf[LINEBUFSIZE];
    memset(buf, 0, LINEBUFSIZE);
    snprintf(buf, LINEBUFSIZE, "Syntax Error at (%d, %d): %s is unexpected", line, column, luxeme);
    except(buf);
}

static int match_next(const char* code, char next, int len, int* current)
{
    if (current == NULL || (*current) > len) {
        return 0;
    }

    if (code[(*current) + 1] != next) {
        return 0;
    }

    (*current)++;
    return 1;
}

Tokenization* toknzr(const char* code)
{
    TokenType type = EOF;
    int len = strlen(code), start = 0, current = 0, line = 1;
    Tokenization* toknz = (Tokenization*)alloc(sizeof(Tokenization));
    toknz->values = list();
    while (current < len) {
        start = current;
        char c = code[current];
        switch (c) {
        case '(':
            list_push(toknz->values, token_simple(LEFT_PAREN, "(", line, current));
            break;
        case ')':
            list_push(toknz->values, token_simple(RIGHT_PAREN, ")", line, current));
            break;
        case '{':
            list_push(toknz->values, token_simple(LEFT_BRACE, "{", line, current));
            break;
        case '}':
            list_push(toknz->values, token_simple(RIGHT_BRACE, "}", line, current));
            break;
        case ',':
            list_push(toknz->values, token_simple(COMMA, ",", line, current));
            break;
        case '.':
            list_push(toknz->values, token_simple(DOT, ".", line, current));
            break;
        case '-':
            list_push(toknz->values, token_simple(MINUS, "-", line, current));
            break;
        case '+':
            list_push(toknz->values, token_simple(PLUS, "+", line, current));
            break;
        case ';':
            list_push(toknz->values, token_simple(SEMICOLON, ";", line, current));
            break;
        case '*':
            list_push(toknz->values, token_simple(STAR, "*", line, current));
            break;
        case '!':
            type = match_next(code, '=', len, &current) ? BANG_EQUAL : BANG;
            list_push(toknz->values, token_simple(type, type != BANG ? "!=" : "!", line, current));
            break;
        case '=':
            type = match_next(code, '=', len, &current) ? EQUAL_EQUAL : EQUAL;
            list_push(toknz->values, token_simple(type, type != EQUAL ? "==" : "=", line, current));
            break;
        case '>':
            type = match_next(code, '=', len, &current) ? GREATER_EQUAL : GREATER;
            list_push(toknz->values, token_simple(type, type != GREATER ? ">=" : ">", line, current));
            break;
        case '<':
            type = match_next(code, '=', len, &current) ? LESS_EQUAL : LESS;
            list_push(toknz->values, token_simple(type, type != LESS ? "<=" : "<", line, current));
            break;
        case '/':
            type = match_next(code, '/', len, &current) ? EOF : SLASH;
            if (type == EOF) {
                do {
                    current++;
                } while (current != len && code[current] != '\n');
            } else {
                list_push(toknz->values, token_simple(SLASH, "/", line, current));
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
            toknzr_error(line, current, "");
            break;
        }
        current++;
    }
    list_push(toknz->values, token_simple(EOF, "", line, current));
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
