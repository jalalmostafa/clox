#ifndef EVAL_H
#define EVAL_H
#include "parse.h"

typedef LiteralExpr Object;

void* visit_binary(void* expr);
void* visit_unary(void* expr);
void* visit_grouping(void* expr);
void* visit_literal(void* expr);

extern ExpressionVisitor EvalVisitor;

#define OPERAND_NUMBER "Operands must be numbers at line: %d"
#define OPERAND_SAMETYPE "Operands must be two numbers or two strings at line: %d"

#endif
