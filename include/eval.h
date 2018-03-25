#ifndef EVAL_H
#define EVAL_H
#include "ds/lldict.h"
#include "parse.h"

typedef LiteralExpr Object;

typedef struct env_t {
    LLDictionary* globalVariables;
    struct env_t* enclosing;
} ExecutionEnvironment;

int env_add_variable(ExecutionEnvironment* env, const char* variableName, Object* obj);
int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj);
Object* env_get_variable_value(ExecutionEnvironment* env, const char* variableName);
void env_destroy(ExecutionEnvironment* env);

void* visit_binary(void* expr);
void* visit_unary(void* expr);
void* visit_grouping(void* expr);
void* visit_literal(void* expr);
void* visit_var_expr(void* expr);
void* visit_assign(void* stmtObj);

void* visit_print(void* stmt);
void* visit_expr(void* stmt);
void* visit_var(void* stmt);
void* visit_block(void* stmt);
void* visit_ifElse(void* stmt);
void* visit_while(void* stmt);
void* visit_for(void* stmt);

extern ExpressionVisitor EvaluateExpressionVisitor;

extern StmtVisitor EvaluateStmtVistior;

extern ExecutionEnvironment GlobalExecutionEnvironment;

#define OPERAND_NUMBER "Syntax Error: Operands must be numbers at line: %d"
#define OPERAND_SAMETYPE "Syntax Error: Operands must be two numbers or two strings at line: %d"

#endif
