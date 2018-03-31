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

extern ExpressionVisitor EvaluateExpressionVisitor;

extern StmtVisitor EvaluateStmtVistior;

extern ExecutionEnvironment GlobalExecutionEnvironment;

#define OPERAND_NUMBER "Syntax Error: Operands must be numbers at line: %d"
#define OPERAND_SAMETYPE "Syntax Error: Operands must be two numbers or two strings at line: %d"

#endif
