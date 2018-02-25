#ifndef EVAL_H
#define EVAL_H
#include "parse.h"

typedef LiteralExpr Object;

void* visit_binary(void* expr);
void* visit_unary(void* expr);
void* visit_grouping(void* expr);
void* visit_literal(void* expr);

void* visit_print(void* stmt);
void* visit_expr(void* stmt);

extern ExpressionVisitor EvaluateExpressionVisitor;

extern StmtVisitor EvaluateStmtVistior;

#define OPERAND_NUMBER "Syntax Error: Operands must be numbers at line: %d"
#define OPERAND_SAMETYPE "Syntax Error: Operands must be two numbers or two strings at line: %d"

#endif
