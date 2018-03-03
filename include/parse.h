#ifndef PARSE_H
#define PARSE_H
#include "tokenizer.h"

typedef enum expr_type_t {
    BINARY = 1,
    GROUPING,
    UNARY,
    LITERAL
} ExpressionType;

typedef enum literal_expr_type_t {
    NIL_L,
    BOOL_L,
    NUMBER_L,
    STRING_L,
    ERROR_L
} LiteralType;

typedef struct expression_t {
    ExpressionType type;
    void* expr;
} Expr;

typedef struct expression_tree_t {
    Expr* expr;
} ExprTree;

typedef struct expression_binary_t {
    Token op;
    Expr* leftExpr;
    Expr* rightExpr;
} BinaryExpr;

typedef struct expression_unary_t {
    Token op;
    Expr* expr;
} UnaryExpr;

typedef struct expression_grouping_t {
    Expr* expr;
} GroupingExpr;

typedef struct expression_literal_t {
    void* value;
    LiteralType type;
    int valueSize;
} LiteralExpr;

typedef void* (*Action)(void*);

typedef struct expression_visitor_t {
    Action visitBinary;
    Action visitUnary;
    Action visitLiteral;
    Action visitGrouping;
} ExpressionVisitor;

typedef enum stmt_type_t {
    STMT_PRINT,
    STMT_EXPR
} StmtType;

typedef struct stmt_visitor_t {
    Action visitPrintStmt;
    Action visitExpressionStmt;
} StmtVisitor;

typedef struct stmt_t {
    StmtType type;
    Expr* expr;
} Stmt;

typedef struct parser_t {
    List* stmts;
} ParsingContext;

ParsingContext parse(Tokenization* toknz);
void* accept(StmtVisitor visitor, Stmt* stmt);
void* accept_expr(ExpressionVisitor visitor, Expr* expr);
void parser_destroy(ParsingContext* ctx);

#define END_OF_TOKENS(x) ((x) == EOF)
#define END_OF_TOKENS(x) ((x) == ENDOFFILE)
#define MATCH(x, type) ((x) == type)

#define UNKNOWN_IDENTIFIER "Unknown Identifier"
#define ERROR_AT_EOF "Syntax Error at end of file: %s\n"
#define ERROR_AT_LINE "Syntax Error (Line %d): %s '%s'\n"
#endif