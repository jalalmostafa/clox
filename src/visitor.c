#include "visitor.h"
#include<stdio.h>

void* accept_expr(ExpressionVisitor visitor, Expr* expr)
{
    switch (expr->type) {
    case LITERAL:
        return visitor.visitLiteral(expr);
    case UNARY:
        return visitor.visitUnary(expr);
    case BINARY:
        return visitor.visitBinary(expr);
    case GROUPING:
        return visitor.visitGrouping(expr);
    case VARIABLE:
        return visitor.visitVariable(expr);
    case ASSIGNMENT:
        return visitor.visitAssignment(expr);
    case LOGICAL:
        return visitor.visitLogical(expr);
    case CALL:
        return visitor.visitCallable(expr);
    }
    return NULL;
}

void* accept(StmtVisitor visitor, Stmt* stmt)
{
    switch (stmt->type) {
    case STMT_PRINT:
        return visitor.visitPrint(stmt);
    case STMT_EXPR:
        return visitor.visitExpression(stmt);
    case STMT_VAR_DECLARATION:
        return visitor.visitVarDeclaration(stmt);
    case STMT_BLOCK:
        return visitor.visitBlock(stmt);
    case STMT_IF_ELSE:
        return visitor.visitIfElse(stmt);
    case STMT_WHILE:
        return visitor.visitWhile(stmt);
    case STMT_FUN:
        return visitor.visitFun(stmt);
    case STMT_RETURN:
        return visitor.visitReturn(stmt);
    }
    return NULL;
}
