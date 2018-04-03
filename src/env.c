#include "ds/lldict.h"
#include "eval.h"
#include "mem.h"
#include <time.h>

static Object* clock_do(List* args, void* decl)
{
    time_t ts = time(NULL);
    double* timestamp = alloc(sizeof(double));
    *timestamp = (double)ts;
    return obj_new(NUMBER_L, timestamp, sizeof(double));
}

static Object* env_clock()
{
    Object* clock = NULL;
    Callable* callableClock = alloc(sizeof(Callable));
    callableClock->arity = 0;
    callableClock->declaration = NULL;
    callableClock->call = clock_do;
    clock = obj_new(CALLABLE_L, callableClock, sizeof(Callable));
    return clock;
}

void env_init_global()
{
    ExecutionEnvironment* env = &GlobalExecutionEnvironment;
    if (env != NULL) {
        if (env->variables == NULL) {
            env->variables = lldict();
        }
    }
    env_add_variable(env, "clock", env_clock());
}

static void env_init(ExecutionEnvironment* env)
{
    if (env != NULL) {
        if (env->variables == NULL) {
            env->variables = lldict();
        }
    }
}

int env_add_variable(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        return lldict_add(env->variables, variableName, obj);
    }
    return 0;
}

int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        if (lldict_contains(env->variables, variableName)) {
            return lldict_set(env->variables, variableName, obj);
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
        obj = (Object*)lldict_get(env->variables, variableName);
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
    lldict_destroy(env->variables);
    env->variables = NULL;
}