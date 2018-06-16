#include "visitor.h"
#include <stdio.h>

void* accept_expr(ExpressionVisitor visitor, Expr* expr)
{
    switch (expr->type) {
    case EXPR_LITERAL:
        return visitor.visitLiteral(expr);
    case EXPR_UNARY:
        return visitor.visitUnary(expr);
    case EXPR_BINARY:
        return visitor.visitBinary(expr);
    case EXPR_GROUPING:
        return visitor.visitGrouping(expr);
    case EXPR_VARIABLE:
        return visitor.visitVariable(expr);
    case EXPR_ASSIGNMENT:
        return visitor.visitAssignment(expr);
    case EXPR_LOGICAL:
        return visitor.visitLogical(expr);
    case EXPR_CALL:
        return visitor.visitCallable(expr);
    case EXPR_GET:
        return visitor.visitGet(expr);
    case EXPR_SET:
        return visitor.visitSet(expr);
    case EXPR_THIS:
        return visitor.visitThis(expr);
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
    case STMT_CLASS:
        return visitor.visitClass(stmt);
    }
    return NULL;
}
