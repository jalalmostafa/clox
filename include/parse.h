#ifndef PARSE_H
#define PARSE_H
#include "tokenizer.h"

typedef enum expr_type_t {
    BINARY = 1,
    GROUPING,
    UNARY,
    LITERAL,
    VARIABLE,
    ASSIGNMENT
} ExpressionType;

typedef enum literal_expr_type_t {
    NIL_L,
    BOOL_L,
    NUMBER_L,
    STRING_L,
    ERROR_L,
    VOID_L
} LiteralType;

typedef struct expression_t {
    ExpressionType type;
    void* expr;
} Expr;

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

typedef struct expression_variable_t {
    Token variableName;
} VariableExpr;

typedef struct expression_literal_t {
    void* value;
    LiteralType type;
    int valueSize;
} LiteralExpr;

typedef struct expression_assignmnt_t {
    Token variableName;
    Expr* rightExpr;
} AssignmentExpr;

typedef void* (*Action)(void*);

typedef struct expression_visitor_t {
    Action visitBinary;
    Action visitUnary;
    Action visitLiteral;
    Action visitGrouping;
    Action visitVariable;
    Action visitAssignment;
} ExpressionVisitor;

typedef enum stmt_type_t {
    STMT_PRINT,
    STMT_VAR_DECLARATION,
    STMT_BLOCK,
    STMT_EXPR,
    STMT_IF_ELSE,
    STMT_FOR,
    STMT_WHILE
} StmtType;

typedef struct stmt_visitor_t {
    Action visitPrint;
    Action visitVarDeclaration;
    Action visitExpression;
    Action visitBlock;
    Action visitIfElse;
    Action visitFor;
    Action visitWhile;
} StmtVisitor;

typedef struct stmt_t {
    StmtType type;
    void* realStmt;
} Stmt;

typedef struct stmt_print_t {
    Expr* expr;
} PrintStmt;

typedef struct stmt_expr_t {
    Expr* expr;
} ExprStmt;

typedef struct stmt_var_declaration_t {
    Expr* initializer;
    Token varName;
} VarDeclarationStmt;

typedef struct stmt_block_t {
    List* innerStmts;
} BlockStmt;

typedef struct stmt_if_t {
    Expr* condition;
    Stmt* thenStmt;
    Stmt* elseStmt;
} IfElseStmt;

typedef struct stmt_while_t {
    Expr* condition;
    Stmt* body;
} WhileStmt;

typedef struct parser_t {
    List* stmts;
} ParsingContext;

ParsingContext parse(Tokenization* toknz);
void* accept(StmtVisitor visitor, Stmt* stmt);
void* accept_expr(ExpressionVisitor visitor, Expr* expr);
void parser_destroy(ParsingContext* ctx);

#define END_OF_TOKENS(x) ((x) == ENDOFFILE)
#define MATCH(x, type) ((x) == type)

#define UNKNOWN_IDENTIFIER "Unresolved Identifier"
#define ERROR_AT_EOF "Syntax Error at end of file: %s\n"
#define ERROR_AT_LINE "Syntax Error (Line %d): %s '%s'\n"
#endif
