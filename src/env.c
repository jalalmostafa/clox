#include "ds/lldict.h"
#include "eval.h"
#include "mem.h"
#include <string.h>
#include <time.h>

static Object* clock_do(List* args, void* decl, ExecutionEnvironment* closure, FunctionType type)
{
    time_t ts = time(NULL);
    double* timestamp = alloc(sizeof(double));
    *timestamp = (double)ts;
    return obj_new(OBJ_NUMBER, timestamp, sizeof(double));
}

static Object* env_clock()
{
    Object* clock = NULL;
    Callable* callableClock = (Callable*)alloc(sizeof(Callable));
    memset(callableClock, 0, sizeof(Callable));
    callableClock->arity = 0;
    callableClock->declaration = NULL;
    callableClock->call = clock_do;
    callableClock->type = FUNCTION_TYPE_FUNCTION;
    clock = obj_new(OBJ_CALLABLE, callableClock, sizeof(Callable));
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

ExecutionEnvironment* env_new()
{
    ExecutionEnvironment* env = (ExecutionEnvironment*)alloc(sizeof(ExecutionEnvironment));
    env->variables = NULL;
    env->enclosing = NULL;
    env_init(env);
    return env;
}

void env_init(ExecutionEnvironment* env)
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
        obj->shallow = 0;
        return lldict_add(env->variables, variableName, obj);
    }
    return 0;
}

int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        if (lldict_contains(env->variables, variableName)) {
            obj->shallow = 0;
            return lldict_set(env->variables, variableName, obj);
        }
    }
    return 0;
}

Object* env_get_variable_value(ExecutionEnvironment* env, const char* variableName)
{
    Object* obj = NULL;
    if (env != NULL) {
        obj = (Object*)lldict_get(env->variables, variableName);
    }
    return obj;
}

static ExecutionEnvironment* env_ancestor(ExecutionEnvironment* env, unsigned int order)
{
    unsigned int i = 0;
    for (i = 0; i < order; i++) {
        env = env->enclosing;
    }
    return env;
}

Object* env_get_variable_value_at(ExecutionEnvironment* env, unsigned int order, const char* variableName)
{
    if (env == NULL) {
        return NULL;
    }
    env = env_ancestor(env, order);
    return env_get_variable_value(env, variableName);
}

int env_set_variable_value_at(ExecutionEnvironment* env, unsigned int order, const char* variableName, Object* value)
{
    if (env == NULL) {
        return 0;
    }

    env = env_ancestor(env, order);
    env_set_variable_value(env, variableName, value);
    return 1;
}

void env_destroy(ExecutionEnvironment* env)
{
    lldict_destroy(env->variables);
    env->variables = NULL;
}
