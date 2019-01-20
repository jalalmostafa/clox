#ifndef TOKNZR_H
#define TOKNZR_H
#include "ds/list.h"

typedef enum tokentype {
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,

    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FUN,
    TOKEN_FOR,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_ENDOFFILE
} TokenType;

typedef struct tokenization {
    List* values;
    int lines;
} Tokenization;

typedef struct token {
    TokenType type;
    char* literal;
    char* lexeme;
    int column;
    int line;
} Token;

Tokenization toknzr(const char* code, int verbose);
void toknzr_destroy(Tokenization toknz);

#define IS_AT_END(x, codeLength) ((x) >= (codeLength))
#define IS_ALPHA_NUMERIC(x) (isalpha((x)) || isdigit((x) || (x) == '_'))

#define AND_KEY "and"
#define CLASS_KEY "class"
#define ELSE_KEY "else"
#define FALSE_KEY "false"
#define FUN_KEY "fun"
#define FOR_KEY "for"
#define IF_KEY "if"
#define NIL_KEY "nil"
#define OR_KEY "or"
#define PRINT_KEY "print"
#define RETURN_KEY "return"
#define SUPER_KEY "super"
#define THIS_KEY "this"
#define TRUE_KEY "true"
#define VAR_KEY "var"
#define WHILE_KEY "while"

#endif
