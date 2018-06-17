#include "eval.h"
#include "ds/lldict.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include "parse.h"
#include "tokenizer.h"
#include "visitor.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void* visit_binary(Expr* expr);
void* visit_unary(Expr* expr);
void* visit_grouping(Expr* expr);
void* visit_literal(Expr* expr);
void* visit_var_expr(Expr* expr);
void* visit_assign(Expr* expr);
void* visit_logical(Expr* expr);
void* visit_callable(Expr* expr);
void* visit_get(Expr* expr);
void* visit_set(Expr* expr);
void* visit_this(Expr* expr);

void* visit_print(Stmt* stmt);
void* visit_expr(Stmt* stmt);
void* visit_var(Stmt* stmt);
void* visit_block(Stmt* stmt);
void* visit_ifElse(Stmt* stmt);
void* visit_while(Stmt* stmt);
void* visit_fun(Stmt* stmt);
void* visit_return(Stmt* stmt);
void* visit_class(Stmt* stmt);

static Object* execute_block(BlockStmt* stmt);
static Object* instance_get(Object* instance, Token name);
static void instance_set(ClassInstance* instance, Token name, Object* value);
static Object* lookup_var(int order, char* name);
static void callable_bind(Object* instanceObj, Callable* method);

ExpressionVisitor EvaluateExpressionVisitor = {
    visit_binary,
    visit_unary,
    visit_literal,
    visit_grouping,
    visit_var_expr,
    visit_assign,
    visit_logical,
    visit_callable,
    visit_get,
    visit_set,
    visit_this
};

StmtVisitor EvaluateStmtVisitor = {
    visit_print,
    visit_var,
    visit_expr,
    visit_block,
    visit_ifElse,
    visit_while,
    visit_fun,
    visit_return,
    visit_class
};

ExecutionEnvironment GlobalExecutionEnvironment = { NULL, NULL };
ExecutionEnvironment* CurrentEnv = &GlobalExecutionEnvironment;

static Object* new_void()
{
    return obj_new(OBJ_VOID, NULL, 0);
}

static Object* new_number(double value)
{
    double* holder = (double*)alloc(sizeof(double));
    *holder = value;
    return obj_new(OBJ_NUMBER, holder, sizeof(double));
}

static Object* new_bool(int truthy)
{
    char* value = (char*)alloc(sizeof(char));
    *value = (char)truthy;
    return obj_new(OBJ_BOOL, value, 1);
}

static Object* eval_expr(Expr* expr)
{
    return (Object*)accept_expr(EvaluateExpressionVisitor, expr);
}

static Object* runtime_error(const char* format, Object** obj, int line, ...)
{
    const char* runtimeError = "Runtime Error (at Line %d): ";
    int len = 0;
    char buffer[LINEBUFSIZE];
    va_list fields;
    memset(buffer, 0, LINEBUFSIZE);
    sprintf(buffer, runtimeError, line);
    va_start(fields, line);
    vsnprintf((char* const)(buffer + strlen(buffer)), LINEBUFSIZE, format, fields);
    va_end(fields);
    len = strlen(buffer) + 1;
    if (*obj == NULL) {
        *obj = (Object*)alloc(sizeof(Object));
    }
    (*obj)->type = OBJ_ERROR;
    (*obj)->value = clone(buffer, len);
    (*obj)->valueSize = 0;
    (*obj)->shallow = 1;
    return *obj;
}

void* visit_binary(Expr* expr)
{
    const BinaryExpr* bexpr = (BinaryExpr*)(expr->expr);
    Object* rObject = eval_expr(bexpr->rightExpr);
    Object* lObject = eval_expr(bexpr->leftExpr);
    Object* result = NULL;
    double *lvalue = NULL, *rvalue = NULL;
    char* svalue = NULL;
    int valueLengthRight = 0, valueLengthLeft = 0;

    if (rObject == NULL || lObject == NULL) {
        return NULL;
    }

    switch (bexpr->op.type) {
    case MINUS:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            result = new_number(*((double*)lObject->value) - *((double*)rObject->value));
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case PLUS:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            result = new_number(*((double*)lObject->value) + *((double*)rObject->value));
        } else if (rObject->type == OBJ_STRING && lObject->type == OBJ_STRING) {
            valueLengthLeft = strlen((char*)lObject->value);
            valueLengthRight = strlen((char*)rObject->value);
            result->type = OBJ_STRING;
            svalue = (char*)alloc(valueLengthLeft + valueLengthRight + 1);
            memcpy(svalue, lObject->value, valueLengthLeft);
            memcpy(svalue + valueLengthLeft, rObject->value, valueLengthRight + 1);
            result->value = svalue;
        } else {
            runtime_error(OPERAND_SAMETYPE, &result, bexpr->op.line);
        }
        break;
    case SLASH:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            result = new_number(*((double*)lObject->value) / *((double*)rObject->value));
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case STAR:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            result = new_number(*((double*)lObject->value) * *((double*)rObject->value));
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case GREATER:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue > *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case GREATER_EQUAL:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue >= *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case LESS:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue < *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case LESS_EQUAL:
        if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue <= *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case EQUAL_EQUAL:
        if (rObject->type == OBJ_NIL && lObject->type == OBJ_NIL) {
            result->value = new_bool(1);
        } else if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue == *rvalue);
        } else if (rObject->type == OBJ_STRING && lObject->type == OBJ_STRING) {
            result = new_bool(strcmp((char*)rObject->value, (char*)lObject->value) == 0);
        } else if (rObject->type == OBJ_BOOL && lObject->type == OBJ_BOOL) {
            result = new_bool(*((char*)rObject->value) == *((char*)lObject->value));
        } else {
            result = new_bool(0);
        }
        break;
    case BANG_EQUAL:
        if (rObject->type == OBJ_NIL && lObject->type == OBJ_NIL) {
            result = new_bool(0);
        } else if (rObject->type == OBJ_NUMBER && lObject->type == OBJ_NUMBER) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue != *rvalue);
        } else if (rObject->type == OBJ_STRING && lObject->type == OBJ_STRING) {
            result = new_bool(strcmp((char*)rObject->value, (char*)lObject->value) != 0);
        } else if (rObject->type == OBJ_BOOL && lObject->type == OBJ_BOOL) {
            result = new_bool(*((char*)rObject->value) != (*(char*)lObject->value));
        } else {
            result = new_bool(1);
        }
        break;
    case AND:
    case OR:
    default:
        break;
    }

    obj_destroy(lObject);
    obj_destroy(rObject);
    return result;
}

void* visit_unary(Expr* expr)
{
    const UnaryExpr* uexpr = (UnaryExpr*)(expr->expr);
    Object* rObject = eval_expr(uexpr->expr), *result = NULL;
    char* bValue = NULL;
    double* value = NULL;
    if (uexpr->op.type == BANG) {
        obj_destroy(rObject);
        bValue = (char*)alloc(sizeof(char));
        *bValue = obj_unlikely(rObject);
        rObject = obj_new(OBJ_BOOL, bValue, 1);
    } else if (uexpr->op.type == MINUS) {
        if (rObject->type != OBJ_NUMBER) {
            runtime_error(OPERAND_NUMBER, &rObject, uexpr->op.line);
        } else {
            value = (double*)clone(rObject->value, sizeof(double));
            *value = -*value;
            obj_destroy(rObject);
            rObject = obj_new(OBJ_NUMBER, value, sizeof(double));
        }
    }
    return rObject;
}

void* visit_grouping(Expr* expr)
{
    const GroupingExpr* gexpr = (GroupingExpr*)(expr->expr);
    return eval_expr(gexpr->expr);
}

void* visit_literal(Expr* expr)
{
    LiteralExpr* original = (LiteralExpr*)(expr->expr);
    char* bValue = NULL;
    switch (original->type) {
    case LITERAL_STRING:
        return obj_new(OBJ_STRING, clone(original->value, original->valueSize), original->valueSize);
    case LITERAL_NUMBER:
        return obj_new(OBJ_NUMBER, clone(original->value, original->valueSize), original->valueSize);
    case LITERAL_NIL:
        return obj_new(OBJ_NIL, clone(original->value, original->valueSize), original->valueSize);
    case LITERAL_BOOL:
        bValue = (char*)alloc(sizeof(char));
        *bValue = (char)(strcmp((char*)original->value, TRUE_KEY) == 0 ? 1 : 0);
        return obj_new(OBJ_BOOL, bValue, 1);
    }
    return NULL;
}

static Object* lookup_var(int order, char* name)
{
    Object* value = NULL;
    if (order == -1) {
        value = env_get_variable_value(&GlobalExecutionEnvironment, name);
    } else {
        value = env_get_variable_value_at(CurrentEnv, order, name);
    }
    return value;
}

void* visit_var_expr(Expr* expr)
{
    VariableExpr* varExpr = (VariableExpr*)(expr->expr);
    Object* value = lookup_var(expr->order, varExpr->variableName.lexeme);
    if (value == NULL) {
        runtime_error("Unresolved variable name '%s'", &value, varExpr->variableName.line, varExpr->variableName.lexeme);
    }
    return value;
}

void* visit_assign(Expr* expr)
{
    AssignmentExpr* assignExpr = (AssignmentExpr*)(expr->expr);
    Object* value = eval_expr(assignExpr->rightExpr);
    if (value == NULL) {
        runtime_error("Cannot assign undeclared variable '%s'", &value, assignExpr->variableName.line, assignExpr->variableName.lexeme);
    } else {
        if (expr->order == 0) {
            env_set_variable_value(&GlobalExecutionEnvironment, assignExpr->variableName.lexeme, value);
        } else {
            env_set_variable_value_at(CurrentEnv, expr->order, assignExpr->variableName.lexeme, value);
        }
    }

    return value;
}

void* visit_logical(Expr* expr)
{
    LogicalExpr* logical = (LogicalExpr*)(expr->expr);
    Object* lvalue = eval_expr(logical->left);
    char lvalueTruth = obj_likely(lvalue);
    if (logical->op.type == OR) {
        if (lvalueTruth) {
            return lvalue;
        }
    } else if (logical->op.type == AND) {
        if (!lvalueTruth) {
            return lvalue;
        }
    }
    return eval_expr(logical->right);
}

void* visit_callable(Expr* expr)
{
    CallExpr* calleeExpr = (CallExpr*)(expr->expr);
    Object* callee = eval_expr(calleeExpr->callee);
    Callable* callable = NULL;
    List* args = list();
    Node* node = NULL;
    Expr* arg = NULL;
    Object* result = NULL;
    if (calleeExpr->args->count != 0) {
        for (node = calleeExpr->args->head; node != NULL; node = node->next) {
            arg = (Expr*)node->data;
            list_push(args, eval_expr(arg));
        }
    }

    if (callee->type != OBJ_CALLABLE && callee->type != OBJ_CLASS_DEFINITION) {
        list_destroy(args);
        return runtime_error("Can only call functions and classes.", &callee, calleeExpr->paren.line);
    }

    callable = callee->type == OBJ_CALLABLE ? (Callable*)callee->value : ((Class*)callee->value)->ctor;

    if (args->count != callable->arity) {
        list_destroy(args);
        return runtime_error("Expected %d but got %d arguments", &callee, calleeExpr->paren.line, args->count, callable->arity);
    }
    result = callable->call(args, callable->declaration, callable->closure, callable->type);

    obj_destroy(callee);
    list_destroy(args);
    return result;
}

void* visit_get(Expr* expr)
{
    GetExpr* get = (GetExpr*)expr->expr;
    Object* obj = eval_expr(get->object);
    if (obj->type == OBJ_CLASS_INSTANCE) {
        return instance_get(obj, get->name);
    }

    return runtime_error("Only instances have properties", &obj, get->name.line);
}

void* visit_set(Expr* expr)
{
    SetExpr* set = (SetExpr*)expr->expr;
    Object *object = eval_expr(set->object), *value = NULL;
    if (object->type != OBJ_CLASS_INSTANCE) {
        return runtime_error("Only instances have fields.", &object, set->name.line);
    }
    value = eval_expr(set->value);
    instance_set(object->value, set->name, value);
    return value;
}

void* visit_this(Expr* expr)
{
    ThisExpr* this = (ThisExpr*)expr->expr;
    return lookup_var(expr->order, this->keyword.lexeme);
}

void* visit_print(Stmt* stmt)
{
    PrintStmt* printStmt = (PrintStmt*)(stmt->realStmt);
    Callable* call = NULL;
    Class* class = NULL;
    ClassInstance* instance = NULL;
    Object* obj = eval_expr(printStmt->expr);
    double* value = NULL;
    char* bValue = NULL;

    switch (obj->type) {
    case OBJ_STRING:
    case OBJ_NIL:
    case OBJ_ERROR:
        printf("%s\n", (char*)obj->value);
        break;
    case OBJ_BOOL:
        bValue = (char*)obj->value;
        printf("%s\n", *bValue == 1 ? TRUE_KEY : FALSE_KEY);
        break;
    case OBJ_NUMBER:
        value = (double*)obj->value;
        if (*value != floor(*value)) {
            printf("%lf\n", *value);
        } else {
            printf("%0.0lf\n", floor(*value));
        }
        break;
    case OBJ_CALLABLE:
        call = (Callable*)obj->value;
        printf("<fn %s>\n", ((FunStmt*)call->declaration)->name.lexeme);
    case OBJ_CLASS_DEFINITION:
        class = (Class*)obj->value;
        printf("<class %s>\n", class->name);
        break;
    case OBJ_CLASS_INSTANCE:
        instance = (ClassInstance*)obj->value;
        printf("<instance %s>\n", instance->type.name);
        break;
    case OBJ_VOID:
        break;
    }
    return obj;
}

void* visit_expr(Stmt* stmt)
{
    ExprStmt* exprStmt = (ExprStmt*)(stmt->realStmt);
    return eval_expr(exprStmt->expr);
}

void* visit_var(Stmt* stmt)
{
    VarDeclarationStmt* varDeclStmt = (VarDeclarationStmt*)(stmt->realStmt);
    Object* value = NULL;
    Token key = varDeclStmt->varName;
    if (varDeclStmt->initializer != NULL) {
        value = eval_expr(varDeclStmt->initializer);
    }
    if (!env_add_variable(CurrentEnv, key.lexeme, value)) {
        runtime_error("'%s' is already defined", &value, key.line, key.lexeme);
    }

    return value;
}

static Object* execute_block(BlockStmt* stmt)
{
    Stmt* innerStmt = NULL;
    Node* node = NULL;
    for (node = stmt->innerStmts->head; node != NULL; node = node->next) {
        innerStmt = (Stmt*)node->data;
        if (innerStmt->type == STMT_RETURN) {
            return accept(EvaluateStmtVisitor, innerStmt);
        } else {
            accept(EvaluateStmtVisitor, innerStmt);
        }
    }
    return new_void();
}

void* visit_block(Stmt* stmt)
{
    BlockStmt* blockStmt = (BlockStmt*)(stmt->realStmt);
    ExecutionEnvironment *prevEnv = CurrentEnv, *env = env_new();
    env->variables = NULL;
    env->enclosing = prevEnv;
    CurrentEnv = env;
    execute_block(blockStmt);
    env_destroy(CurrentEnv);
    CurrentEnv = prevEnv;
    env_destroy(env);
    fr(env);
    return new_void();
}

void* visit_ifElse(Stmt* stmt)
{
    IfElseStmt* ifElseStmt = (IfElseStmt*)(stmt->realStmt);
    Object* eval_exprCond = eval_expr(ifElseStmt->condition);
    if (obj_likely(eval_exprCond)) {
        return accept(EvaluateStmtVisitor, ifElseStmt->thenStmt);
    } else if (ifElseStmt->elseStmt != NULL) {
        return accept(EvaluateStmtVisitor, ifElseStmt->elseStmt);
    }
    return new_void();
}

void* visit_while(Stmt* stmt)
{
    WhileStmt* whileStmt = (WhileStmt*)(stmt->realStmt);
    while (obj_likely(eval_expr(whileStmt->condition))) {
        accept(EvaluateStmtVisitor, whileStmt->body);
    }

    return new_void();
}

static Object* fun_call(List* args, void* declaration, ExecutionEnvironment* closure, FunctionType type)
{
    FunStmt* funDecl = (FunStmt*)declaration;
    Token* tkn = NULL;
    Node *node = NULL, *valueWrapper = NULL;
    int i = 0;
    Object* value = NULL;
    ExecutionEnvironment *prevEnv = CurrentEnv, *env = closure;
    env->enclosing = prevEnv;
    CurrentEnv = env;

    for (node = funDecl->args->head; node != NULL; node = node->next) {
        tkn = (Token*)node->data;
        valueWrapper = list_at(args, i);
        if (valueWrapper != NULL) {
            value = (Object*)valueWrapper->data;
            env_add_variable(CurrentEnv, tkn->lexeme, value);
        }
        i++;
    }

    value = execute_block((BlockStmt*)funDecl->body->realStmt);
    if (value->type == OBJ_VOID) {
        if (type == FUNCTION_TYPE_CTOR) {
            obj_destroy(value);
            value = env_get_variable_value(CurrentEnv, "this");
        }
    }

    if (type == FUNCTION_TYPE_CTOR) {
        obj_destroy(value);
        value = env_get_variable_value(CurrentEnv, "this");
    }

    CurrentEnv = prevEnv;
    return value;
}

static Object* build_function(FunStmt* funStmt, ExecutionEnvironment* closure, FunctionType type)
{
    Callable* call = (Callable*)alloc(sizeof(Callable));
    memset(call, 0, sizeof(Callable));
    call->call = fun_call;
    call->arity = funStmt->args->count;
    call->declaration = (void*)funStmt;
    call->closure = closure;
    call->type = type;
    return obj_new(OBJ_CALLABLE, call, sizeof(Callable));
}

void* visit_fun(Stmt* stmt)
{
    FunStmt* funStmt = (FunStmt*)(stmt->realStmt);
    Object* obj = NULL;
    obj = build_function(funStmt, CurrentEnv, FUNCTION_TYPE_FUNCTION);
    env_add_variable(CurrentEnv, funStmt->name.lexeme, obj);
    return new_void();
}

void* visit_return(Stmt* stmt)
{
    Object* value = new_void();
    ReturnStmt* returnStmt = (ReturnStmt*)(stmt->realStmt);
    if (returnStmt->value != NULL) {
        obj_destroy(value);
        value = eval_expr(returnStmt->value);
    }

    return value;
}

static Object* instantiate(List* args, void* declaration, ExecutionEnvironment* env, FunctionType funType)
{
    Class* type = (Class*)declaration;
    ClassInstance* instance = (ClassInstance*)alloc(sizeof(ClassInstance));
    Object* instanceObj = obj_new(OBJ_CLASS_INSTANCE, instance, sizeof(ClassInstance));
    Object* customCtorMethod = lldict_get(type->methods, "init");
    Callable* customCtor = NULL;

    if (customCtor != NULL) {
        customCtor = (Callable*)customCtorMethod->value;
        callable_bind(instanceObj, customCtor);
        customCtor->call(args, declaration, env, customCtor->type);
    }

    instance->type = *type;
    instance->fields = lldict();
    return instanceObj;
}

void* visit_class(Stmt* stmt)
{
    Node* n = NULL;
    FunStmt* funStmt = NULL;
    ClassStmt* classStmt = (ClassStmt*)stmt->realStmt;
    Class* class = (Class*)alloc(sizeof(Class));
    Callable *ctor = (Callable*)alloc(sizeof(Callable)), *customCtor = NULL;
    Object* callable = NULL;

    ctor->arity = 0;
    ctor->type = FUNCTION_TYPE_CTOR;
    ctor->closure = CurrentEnv;
    ctor->declaration = class;
    ctor->call = instantiate;
    class->name = clone(classStmt->name.lexeme, strlen(classStmt->name.lexeme) + 1);
    class->ctor = ctor;
    class->methods = lldict();

    for (n = classStmt->methods->head; n != NULL; n = n->next) {
        funStmt = (FunStmt*)((Stmt*)n->data)->realStmt;
        callable = build_function(funStmt, CurrentEnv, FUNCTION_TYPE_METHOD);
        lldict_add(class->methods, funStmt->name.lexeme, callable);
    }

    customCtor = lldict_get(class->methods, "init");
    if (customCtor != NULL) {
        ctor->arity = customCtor->arity;
        customCtor->type = FUNCTION_TYPE_CTOR;
    }

    env_add_variable(CurrentEnv, class->name, obj_new(OBJ_CLASS_DEFINITION, class, sizeof(Class)));
    return new_void();
}

static void callable_destroy(Callable* callable)
{
    if (GlobalExecutionEnvironment.variables != callable->closure->variables) {
        env_destroy(callable->closure);
        fr(callable->closure);
    }
}

void obj_destroy(Object* obj)
{
    Callable* callable = NULL;
    Class* class = NULL;
    ClassInstance* instance = NULL;

    if (obj != NULL && obj->shallow == 1) {
        switch (obj->type) {
        case OBJ_NIL:
        case OBJ_VOID:
            break;
        case OBJ_BOOL:
        case OBJ_NUMBER:
        case OBJ_STRING:
        case OBJ_ERROR:
            fr(obj->value);
            break;
        case OBJ_CALLABLE:
            callable = (Callable*)obj->value;
            callable_destroy(callable);
            fr(obj->value);
            break;
        case OBJ_CLASS_DEFINITION:
            class = (Class*)obj->value;
            callable_destroy(class->ctor);
            fr(class->ctor);
            fr(class->name);
            fr(obj->value);
            break;
        case OBJ_CLASS_INSTANCE:
            instance = (ClassInstance*)obj->value;
            lldict_destroy(instance->fields);
            fr(instance);
            break;
        default:
            runtime_error("Unknown Object to destroy", &obj, 0);
            break;
        }
        fr(obj);
    }
}

Object* obj_new(ObjectType type, void* value, int valueSize)
{
    Object* obj = alloc(sizeof(Object));
    obj->type = type;
    obj->value = value;
    obj->valueSize = valueSize;
    obj->shallow = 1;
    return obj;
}

char obj_likely(Object* obj)
{
    char* value = NULL;
    if (obj->type == OBJ_NIL) {
        return (char)0;
    }

    if (obj->type == OBJ_BOOL) {
        value = (char*)obj->value;
        return *value == (char)1;
    }

    return (char)1;
}

char obj_unlikely(Object* obj)
{
    char* value = NULL;
    if (obj->type == OBJ_NIL) {
        return (char)0;
    }

    if (obj->type == OBJ_BOOL) {
        value = (char*)obj->value;
        return *value != (char)1;
    }

    return (char)0;
}

void eval(Stmt* stmt)
{
    accept(EvaluateStmtVisitor, stmt);
}

static void callable_bind(Object* instanceObj, Callable* method)
{
    ExecutionEnvironment* classEnv = env_new();
    env_add_variable(classEnv, "this", instanceObj);
    classEnv->enclosing = method->closure;
    method->closure = classEnv;
}

static Object* instance_get(Object* instanceObj, Token name)
{
    Object* member = NULL;
    ClassInstance* instance = (ClassInstance*)instanceObj->value;
    Callable* method = NULL;
    if (lldict_contains(instance->fields, name.lexeme)) {
        return lldict_get(instance->fields, name.lexeme);
    }

    if (lldict_contains(instance->type.methods, name.lexeme)) {
        member = lldict_get(instance->type.methods, name.lexeme);
        method = (Callable*)member->value;
        callable_bind(instanceObj, method);
        return member;
    }

    return runtime_error("Undefined property '%s'", NULL, name.line, name.lexeme);
}

static void instance_set(ClassInstance* instance, Token name, Object* value)
{
    if (lldict_contains(instance->fields, name.lexeme)) {
        lldict_set(instance->fields, name.lexeme, value);
    } else {
        lldict_add(instance->fields, name.lexeme, value);
    }
}
