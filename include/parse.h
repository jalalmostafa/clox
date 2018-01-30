#ifndef PARSE_H
#define PARSE_H
#include "tokenizer.h"

typedef enum expr_type_t {
    BINARY = 1,
    GROUPING,
    UNARY,
    LITERAL
} ExpressionType;

typedef struct expression_t {
    ExpressionType type;
    void* expr;
} Expr;

typedef struct expression_tree {
    Expr* expr;
} ExprTree;

typedef struct expression_binary {
    Token op;
    Expr* leftExpr;
    Expr* rightExpr;
} BinaryExpr;

typedef struct expression_unary {
    Token op;
    Expr* expr;
} UnaryExpr;

typedef struct expression_grouping {
    Expr* expr;
} GroupingExpr;

typedef struct expression_literal {
    void* value;
} LiteralExpr;

typedef void* (*Action)(void*);

Expr* parse(Tokenization* toknz);
void destroy_expr(Expr* expr);

#define END_OF_TOKENS(x) ((x) == EOF)
#define MATCH(x, type) ((x) == type)
#endif