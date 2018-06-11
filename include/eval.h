#ifndef EVAL_H
#define EVAL_H
#include "ds/lldict.h"
#include "parse.h"

typedef LiteralExpr Object;

typedef struct env_t {
    LLDictionary* variables;
    struct env_t* enclosing;
} ExecutionEnvironment;

typedef Object* (*CallFunc)(List* args, void* declaration, ExecutionEnvironment env);

typedef struct callable_t {
    unsigned int arity;
    CallFunc call;
    void* declaration;
    ExecutionEnvironment closure;
} Callable;

void env_init(ExecutionEnvironment* env);
void env_init_global();
int env_add_variable(ExecutionEnvironment* env, const char* variableName, Object* obj);
int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj);
Object* env_get_variable_value(ExecutionEnvironment* env, const char* variableName);
void env_destroy(ExecutionEnvironment* env);
int env_set_variable_value_at(ExecutionEnvironment* env, unsigned int order, const char* variableName, Object* value);
Object* env_get_variable_value_at(ExecutionEnvironment* env, unsigned int order, const char* variableName);

void obj_destroy(Object* obj);
Object* obj_new(LiteralType type, void* value, int valueSize);

extern ExecutionEnvironment GlobalExecutionEnvironment;

void eval(Stmt* stmt);

#define OPERAND_NUMBER "Syntax Error: Operands must be numbers at line: %d"
#define OPERAND_SAMETYPE "Syntax Error: Operands must be two numbers or two strings at line: %d"
#endif
