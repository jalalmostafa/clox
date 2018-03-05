#include "ds/lldict.h"
#include "eval.h"
#include "mem.h"

int env_add_variable(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    if (env != NULL) {
        if (env->globalVariables == NULL) {
            env->globalVariables = lldict();
        }
        return lldict_add(env->globalVariables, variableName, obj);
    }
    return 0;
}

int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    if (env != NULL) {
        if (env->globalVariables == NULL) {
            env->globalVariables = lldict();
        }
        lldict_set(env->globalVariables, variableName, obj);
    }
    return 0;
}

Object* env_get_variable_value(ExecutionEnvironment* env, const char* variableName)
{
    if (env != NULL && env->globalVariables != NULL) {
        return (Object*)lldict_get(env->globalVariables, variableName);
    }
    return NULL;
}

void env_destroy(ExecutionEnvironment* env)
{
    lldict_destroy(env->globalVariables);
    env->globalVariables = NULL;
}