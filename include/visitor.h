#ifndef VISITOR_H
#define VISITOR_H
#include "parse.h"

typedef void* (*ActionExpr)(Expr*);
typedef void* (*ActionStmt)(Stmt* stmt);

typedef struct expression_visitor_t {
    ActionExpr visitBinary;
    ActionExpr visitUnary;
    ActionExpr visitLiteral;
    ActionExpr visitGrouping;
    ActionExpr visitVariable;
    ActionExpr visitAssignment;
    ActionExpr visitLogical;
    ActionExpr visitCallable;
} ExpressionVisitor;

typedef struct stmt_visitor_t {
    ActionStmt visitPrint;
    ActionStmt visitVarDeclaration;
    ActionStmt visitExpression;
    ActionStmt visitBlock;
    ActionStmt visitIfElse;
    ActionStmt visitWhile;
    ActionStmt visitFun;
    ActionStmt visitReturn;
} StmtVisitor;

void* accept(StmtVisitor visitor, Stmt* stmt);
void* accept_expr(ExpressionVisitor visitor, Expr* expr);

#endif
