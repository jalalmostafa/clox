#include "ds/lldict.h"
#include "eval.h"
#include "mem.h"

static void env_init(ExecutionEnvironment* env)
{
    if (env != NULL) {
        if (env->globalVariables == NULL) {
            env->globalVariables = lldict();
        }
        if (env->enclosing == NULL) {
            env->enclosing = NULL;
        }
    }
}

int env_add_variable(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        return lldict_add(env->globalVariables, variableName, obj);
    }
    return 0;
}

int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        if (lldict_contains(env->globalVariables, variableName)) {
            return lldict_set(env->globalVariables, variableName, obj);
        }
        if (env->enclosing != NULL) {
            return env_set_variable_value(env->enclosing, variableName, obj);
        }
    }
    return 0;
}

Object* env_get_variable_value(ExecutionEnvironment* env, const char* variableName)
{
    Object *obj = NULL, *clonedObj = NULL;
    if (env != NULL) {
        obj = (Object*)lldict_get(env->globalVariables, variableName);
        if (env->enclosing != NULL && obj == NULL) {
            return env_get_variable_value(env->enclosing, variableName);
        }
    }
    clonedObj = (Object*)clone(obj, sizeof(Object));
    clonedObj->value = clone(obj->value, obj->valueSize);
    return clonedObj;
}

void env_destroy(ExecutionEnvironment* env)
{
    lldict_destroy(env->globalVariables);
    env->globalVariables = NULL;
}