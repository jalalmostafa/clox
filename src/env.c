#include "ds/dict.h"
#include "eval.h"
#include "global.h"
#include "interp.h"
#include "mem.h"
#include <string.h>
#include <time.h>

static int env_clear_values(KeyValuePair* pair);

static Callable* callable_new(int arity, CallFunc func)
{
    Callable* callable = (Callable*)alloc(sizeof(Callable));
    callable->arity = arity;
    callable->declaration = NULL;
    callable->call = func;
    callable->type = FUNCTION_TYPE_FUNCTION;
    callable->closure = NULL;
    return callable;
}

static Object* clock_do(List* args, void* decl, ExecutionEnvironment* closure, FunctionType type)
{
    time_t ts = time(NULL);
    double* timestamp = alloc(sizeof(double));
    *timestamp = (double)ts;
    return obj_new(OBJ_NUMBER, timestamp, sizeof(double));
}

static Object* env_clock()
{
    Callable* callableClock = callable_new(0, clock_do);
    return obj_new(OBJ_CALLABLE, callableClock, sizeof(Callable));
}

static int is_number(char* str)
{
    while (*str != '\0') {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        str++;
    }
    return 1;
}

static Object* read_do(List* args, void* decl, ExecutionEnvironment* closure, FunctionType type)
{
    char* boolOrString = NULL;
    double* number = NULL;
    int length = 0;
    char input[LINEBUFSIZE];
    memset(input, 0, LINEBUFSIZE);
    fgets(input, LINEBUFSIZE, stdin);
    length = strlen(input);
    input[length - 1] = 0;
    length--;
    if (strcmp(input, NIL_KEY) == 0) {
        return obj_new(OBJ_NIL, NULL, 0);
    } else if (strcmp(input, TRUE_KEY) == 0) {
        boolOrString = (char*)alloc(sizeof(char));
        *boolOrString = 1;
        return obj_new(OBJ_BOOL, boolOrString, sizeof(char));
    } else if (strcmp(input, FALSE_KEY) == 0) {
        boolOrString = (char*)alloc(sizeof(char));
        *boolOrString = 0;
        return obj_new(OBJ_BOOL, boolOrString, sizeof(char));
    } else if (is_number(input)) {
        number = (double*)alloc(sizeof(double));
        *number = atof(input);
        return obj_new(OBJ_NUMBER, number, sizeof(double));
    }

    boolOrString = (char*)alloc(length + 1);
    memcpy(boolOrString, input, length + 1);
    return obj_new(OBJ_STRING, boolOrString, length + 1);
}

static Object* env_read()
{
    Callable* callableRead = callable_new(0, read_do);
    return obj_new(OBJ_CALLABLE, callableRead, sizeof(Callable));
}

void env_init_global()
{
    ExecutionEnvironment* env = &GlobalExecutionEnvironment;
    if (env != NULL) {
        if (env->variables == NULL) {
            env->variables = dict(env_clear_values);
        }
    }
    env_add_variable(env, "clock", env_clock());
    env_add_variable(env, "read", env_read());
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
            env->variables = dict(env_clear_values);
        }
    }
}

int env_add_variable(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        obj->shallow = 0;
        return dict_add(env->variables, variableName, obj);
    }
    return 0;
}

int env_set_variable_value(ExecutionEnvironment* env, const char* variableName, Object* obj)
{
    env_init(env);
    if (env != NULL) {
        if (dict_contains(env->variables, variableName)) {
            obj->shallow = 0;
            return dict_set(env->variables, variableName, obj);
        }
    }
    return 0;
}

Object* env_get_variable_value(ExecutionEnvironment* env, const char* variableName)
{
    Object* obj = NULL;
    if (env != NULL) {
        obj = (Object*)dict_get(env->variables, variableName);
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

static int env_clear_values(KeyValuePair* pair)
{
    return obj_force_destroy(pair);
}

void env_destroy(ExecutionEnvironment* env)
{
    dict_destroy(env->variables);
    env->variables = NULL;
}
