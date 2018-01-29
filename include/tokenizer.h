#ifndef TOKNZR_H
#define TOKNZR_H
#include "ds/list.h"

typedef enum tokentype {
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    STAR,

    // One character probably followed by an Equal character.
    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,

    // Literals.
    IDENTIFIER,
    STRING,
    NUMBER,

    // Keywords.
    AND,
    CLASS,
    ELSE,
    FALSE,
    FUN,
    FOR,
    IF,
    NIL,
    OR,
    PRINT,
    RETURN,
    SUPER,
    THIS,
    TRUE,
    VAR,
    WHILE,

    EOF
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

Tokenization* toknzr(const char* code);
void toknzr_destroy(Tokenization* toknz);

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
