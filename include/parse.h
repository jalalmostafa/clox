#ifndef PARSE_H
#define PARSE_H
#include "tokenizer.h"

typedef enum expr_type_t {
    EXPR_BINARY = 1,
    EXPR_GROUPING,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_VARIABLE,
    EXPR_ASSIGNMENT,
    EXPR_LOGICAL,
    EXPR_CALL,
    EXPR_GET,
    EXPR_SET,
    EXPR_THIS,
    EXPR_SUPER
} ExpressionType;

typedef enum literal_expr_type_t {
    LITERAL_NIL,
    LITERAL_BOOL,
    LITERAL_NUMBER,
    LITERAL_STRING
} LiteralType;

typedef struct expression_t {
    ExpressionType type;
    void* expr;
    unsigned int order;
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

typedef struct expression_logical_t {
    Expr* left;
    Expr* right;
    Token op;
} LogicalExpr;

typedef struct expression_call_t {
    Expr* callee;
    Token paren;
    List* args;
} CallExpr;

typedef struct expression_get_t {
    Expr* object;
    Token name;
} GetExpr;

typedef struct expression_set_t {
    Expr* object;
    Token name;
    Expr* value;
} SetExpr;

typedef struct expression_this_t {
    Token keyword;
} ThisExpr;

typedef struct expression_super_t {
    Token keyword;
    Token method;
} SuperExpr;

typedef enum stmt_type_t {
    STMT_PRINT,
    STMT_VAR_DECLARATION,
    STMT_BLOCK,
    STMT_EXPR,
    STMT_IF_ELSE,
    STMT_WHILE,
    STMT_FUN,
    STMT_RETURN,
    STMT_CLASS
} StmtType;

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

typedef struct stmt_fun_t {
    List* args;
    Token name;
    Stmt* body;
} FunStmt;

typedef struct stmt_return_t {
    Token keyword;
    Expr* value;
} ReturnStmt;

typedef struct stmt_class_t {
    Token name;
    List* methods;
    Expr* super;
} ClassStmt;

typedef struct parser_t {
    List* stmts;
    Expr* expr;
} ParsingContext;

ParsingContext parse(Tokenization toknz);
ParsingContext parse_literal(Tokenization toknz);
void parser_destroy(ParsingContext* ctx);

#define END_OF_TOKENS(x) ((x) == ENDOFFILE)
#define MATCH(x, type) ((x) == type)

#define UNKNOWN_IDENTIFIER "Unresolved Identifier"
#define ERROR_AT_EOF "Syntax Error at end of file: %s\n"
#define ERROR_AT_LINE "Syntax Error (Line %d): %s '%s'\n"
#define MAX_ARGS 8

#endif
