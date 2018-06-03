#include "eval.h"
#include "ds/lldict.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include "parse.h"
#include "tokenizer.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int obj_likely(Object* obj);
Object* obj_new(LiteralType type, void* value, int valueSize);
void obj_destroy(Object* obj);

void* visit_binary(Expr* expr);
void* visit_unary(Expr* expr);
void* visit_grouping(Expr* expr);
void* visit_literal(Expr* expr);
void* visit_var_expr(Expr* expr);
void* visit_assign(Expr* expr);
void* visit_logical(Expr* expr);
void* visit_callable(Expr* expr);

void* visit_print(Stmt* stmt);
void* visit_expr(Stmt* stmt);
void* visit_var(Stmt* stmt);
void* visit_block(Stmt* stmt);
void* visit_ifElse(Stmt* stmt);
void* visit_while(Stmt* stmt);
void* visit_fun(Stmt* stmt);
void* visit_return(Stmt* stmt);

static Object* execute_block(BlockStmt* stmt);

ExpressionVisitor EvaluateExpressionVisitor = {
    visit_binary,
    visit_unary,
    visit_literal,
    visit_grouping,
    visit_var_expr,
    visit_assign,
    visit_logical,
    visit_callable
};

StmtVisitor EvaluateStmtVisitor = {
    visit_print,
    visit_var,
    visit_expr,
    visit_block,
    visit_ifElse,
    visit_while,
    visit_fun,
    visit_return
};

ExecutionEnvironment GlobalExecutionEnvironment = { NULL, NULL };
ExecutionEnvironment* CurrentEnv = &GlobalExecutionEnvironment;

static Object* new_void()
{
    Object* obj = (Object*)alloc(sizeof(Object));
    obj->type = VOID_L;
    obj->value = NULL;
    obj->valueSize = 0;
    return obj;
}

static Object* new_number(double value)
{
    double* holder = NULL;
    Object* result = NULL;
    result = (Object*)alloc(sizeof(Object));
    memset(result, 0, sizeof(Object));
    result->type = NUMBER_L;
    result->value = alloc(sizeof(double));
    result->valueSize = sizeof(double);
    holder = (double*)result->value;
    *holder = value;
    return result;
}

static Object* new_bool(int truthy)
{
    Object* result = NULL;
    const char* truthyValue = truthy ? TRUE_KEY : FALSE_KEY;
    int valueSize = strlen(truthyValue) + 1;
    result = (Object*)alloc(sizeof(Object));
    memset(result, 0, sizeof(Object));
    result->type = BOOL_L;
    result->value = clone((void*)truthyValue, valueSize);
    result->valueSize = valueSize;
    return result;
}

static Object* eval(Expr* expr)
{
    return (Object*)accept_expr(EvaluateExpressionVisitor, expr);
}

static const char* expr_unlikely(LiteralExpr* expr)
{
    if (expr->type == NIL_L) {
        return TRUE_KEY;
    }

    if (expr->type == BOOL_L) {
        return strcmp(TRUE_KEY, (const char*)expr->value) != 0 ? TRUE_KEY : FALSE_KEY;
    }

    return FALSE_KEY;
}

static const char* expr_likely(LiteralExpr* expr)
{
    if (expr->type == NIL_L) {
        return FALSE_KEY;
    }

    if (expr->type == BOOL_L) {
        return strcmp(TRUE_KEY, (const char*)expr->value) == 0 ? TRUE_KEY : FALSE_KEY;
    }

    return TRUE_KEY;
}

static const char* likely(int condition)
{
    if (condition > 0) {
        return TRUE_KEY;
    }
    return FALSE_KEY;
}

static Object* runtime_error(const char* format, Object** obj, int line, ...)
{
    const char* runtimeError = "Runtime Error (at Line %d): ";
    int len = 0;
    char buffer[LINEBUFSIZE];
    memset(buffer, 0, LINEBUFSIZE);
    sprintf(buffer, runtimeError, line);
    va_list fields;
    va_start(fields, line);
    vsnprintf((char* const)(buffer + strlen(buffer)), LINEBUFSIZE, format, fields);
    va_end(fields);
    len = strlen(buffer) + 1;
    if (*obj == NULL) {
        *obj = (Object*)alloc(sizeof(Object));
    }
    (*obj)->type = ERROR_L;
    (*obj)->value = clone(buffer, len);
    (*obj)->valueSize = 0;
    return *obj;
}

void* visit_binary(Expr* expr)
{
    const BinaryExpr* bexpr = (BinaryExpr*)(expr->expr);
    Object* rObject = eval(bexpr->rightExpr);
    Object* lObject = eval(bexpr->leftExpr);
    Object* result = NULL;
    double *lvalue = NULL, *rvalue = NULL;
    char* svalue = NULL;
    int valueLengthRight = 0, valueLengthLeft = 0;

    if (rObject == NULL || lObject == NULL) {
        return NULL;
    }

    switch (bexpr->op.type) {
    case MINUS:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result = new_number(*((double*)lObject->value) - *((double*)rObject->value));
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case PLUS:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result = new_number(*((double*)lObject->value) + *((double*)rObject->value));
        } else if (rObject->type == STRING_L && lObject->type == STRING_L) {
            valueLengthLeft = strlen((char*)lObject->value);
            valueLengthRight = strlen((char*)rObject->value);
            result->type = STRING_L;
            svalue = (char*)alloc(valueLengthLeft + valueLengthRight + 1);
            memcpy(svalue, lObject->value, valueLengthLeft);
            memcpy(svalue + valueLengthLeft, rObject->value, valueLengthRight + 1);
            result->value = svalue;
        } else {
            runtime_error(OPERAND_SAMETYPE, &result, bexpr->op.line);
        }
        break;
    case SLASH:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result = new_number(*((double*)lObject->value) / *((double*)rObject->value));
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case STAR:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result = new_number(*((double*)lObject->value) * *((double*)rObject->value));
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case GREATER:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue > *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case GREATER_EQUAL:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue >= *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case LESS:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue < *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case LESS_EQUAL:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue <= *rvalue);
        } else {
            runtime_error(OPERAND_NUMBER, &result, bexpr->op.line);
        }
        break;
    case EQUAL_EQUAL:
        if (rObject->type == NIL_L && lObject->type == NIL_L) {
            result->value = new_bool(1);
        } else if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue == *rvalue);
        } else if ((rObject->type == STRING_L && lObject->type == STRING_L)
            || (rObject->type == BOOL_L && lObject->type == BOOL_L)) {
            result = new_bool(strcmp((char*)rObject->value, (char*)lObject->value) == 0);
        } else {
            result = new_bool(0);
        }
        break;
    case BANG_EQUAL:
        if (rObject->type == NIL_L && lObject->type == NIL_L) {
            result = new_bool(0);
        } else if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            result = new_bool(*lvalue != *rvalue);
        } else if ((rObject->type == STRING_L && lObject->type == STRING_L)
            || (rObject->type == BOOL_L && lObject->type == BOOL_L)) {
            result = new_bool(strcmp((char*)rObject->value, (char*)lObject->value) != 0);
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
    Object* rObject = eval(uexpr->expr);
    const char* st = NULL;
    double* value = NULL;
    switch (uexpr->op.type) {
    case BANG:
        st = expr_unlikely(rObject);
        fr(rObject->value);
        rObject->type = BOOL_L;
        rObject->value = clone((void*)st, strlen(st) + 1);
        break;
    case MINUS:
        if (rObject->type != NUMBER_L) {
            runtime_error(OPERAND_NUMBER, &rObject, uexpr->op.line);
        } else {
            value = (double*)clone(rObject->value, sizeof(double));
            *value = -*value;
            fr(rObject->value);
            rObject->value = value;
        }
        break;
    }
    return rObject;
}

void* visit_grouping(Expr* expr)
{
    const GroupingExpr* gexpr = (GroupingExpr*)(expr->expr);
    return eval(gexpr->expr);
}

void* visit_literal(Expr* expr)
{
    LiteralExpr* original = (LiteralExpr*)(expr->expr);
    Object* result = (Object*)clone(original, sizeof(LiteralExpr));
    result->value = clone(original->value, original->valueSize);
    return result;
}

void* visit_var_expr(Expr* expr)
{
    VariableExpr* varExpr = (VariableExpr*)(expr->expr);
    Object* value = env_get_variable_value(CurrentEnv, varExpr->variableName.lexeme);
    if (value == NULL) {
        runtime_error("Unresolved variable name '%s'", &value, varExpr->variableName.line, varExpr->variableName.lexeme);
    }
    return value;
}

void* visit_assign(Expr* expr)
{
    AssignmentExpr* assignExpr = (AssignmentExpr*)(expr->expr);
    Object* value = eval(assignExpr->rightExpr);

    if (value == NULL) {
        runtime_error("Cannot assign undeclared variable '%s'", &value, assignExpr->variableName.line, assignExpr->variableName.lexeme);
    } else {
        env_set_variable_value(CurrentEnv, assignExpr->variableName.lexeme, value);
    }

    return value;
}

void* visit_logical(Expr* expr)
{
    LogicalExpr* logical = (LogicalExpr*)(expr->expr);
    Object* lvalue = eval(logical->left);
    int lvalueTruth = obj_likely(lvalue);
    if (logical->op.type == OR) {
        if (lvalueTruth) {
            return lvalue;
        }
    } else if (logical->op.type == AND) {
        if (!lvalueTruth) {
            return lvalue;
        }
    }
    return eval(logical->right);
}

void* visit_callable(Expr* expr)
{
    CallExpr* calleeExpr = (CallExpr*)(expr->expr);
    Object* callee = eval(calleeExpr->callee);
    Callable* callable = NULL;
    List* args = list();
    Node* node = NULL;
    Expr* arg = NULL;
    Object* result = NULL;
    if (calleeExpr->args->count != 0) {
        for (node = calleeExpr->args->head; node != NULL; node = node->next) {
            arg = (Expr*)node->data;
            list_push(args, eval(arg));
        }
    }

    if (callee->type != CALLABLE_L) {
        list_destroy(args);
        return runtime_error("Can only call functions and classes.", &callee, calleeExpr->paren.line);
    }

    callable = (Callable*)callee->value;

    if (args->count != callable->arity) {
        list_destroy(args);
        return runtime_error("Expected %d but got %d arguments", &callee, calleeExpr->paren.line, args->count, callable->arity);
    }

    result = callable->call(args, callable->declaration, callable->closure);
    obj_destroy(callee);
    list_destroy(args);
    return result;
}

/// Statements Evaluation

void* visit_print(Stmt* stmt)
{
    PrintStmt* printStmt = (PrintStmt*)(stmt->realStmt);
    Callable* call = NULL;
    Object* obj = eval(printStmt->expr);
    double* value = NULL;
    switch (obj->type) {
    case STRING_L:
    case BOOL_L:
    case NIL_L:
    case ERROR_L:
        printf("%s\n", (char*)obj->value);
        break;
    case NUMBER_L:
        value = (double*)obj->value;
        if (*value != floor(*value)) {
            printf("%lf\n", *value);
        } else {
            printf("%0.0lf\n", floor(*value));
        }
        break;
    case CALLABLE_L:
        call = (Callable*)obj->value;
        printf("<fn %s>\n", ((FunStmt*)call->declaration)->name.lexeme);
    case VOID_L:
        break;
    }
    return obj;
}

void* visit_expr(Stmt* stmt)
{
    ExprStmt* exprStmt = (ExprStmt*)(stmt->realStmt);
    return eval(exprStmt->expr);
}

void* visit_var(Stmt* stmt)
{
    VarDeclarationStmt* varDeclStmt = (VarDeclarationStmt*)(stmt->realStmt);
    Object* value = NULL;
    Token key = varDeclStmt->varName;
    if (varDeclStmt->initializer != NULL) {
        value = eval(varDeclStmt->initializer);
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
    Stmt* innerStmt = NULL;
    Node* node = NULL;
    ExecutionEnvironment *prevEnv = CurrentEnv, env = { NULL, prevEnv };
    CurrentEnv = &env;
    execute_block(blockStmt);
    env_destroy(CurrentEnv);
    CurrentEnv = prevEnv;
    return new_void();
}

void* visit_ifElse(Stmt* stmt)
{
    IfElseStmt* ifElseStmt = (IfElseStmt*)(stmt->realStmt);
    Object* evalCond = eval(ifElseStmt->condition);
    if (obj_likely(evalCond)) {
        return accept(EvaluateStmtVisitor, ifElseStmt->thenStmt);
    } else if (ifElseStmt->elseStmt != NULL) {
        return accept(EvaluateStmtVisitor, ifElseStmt->elseStmt);
    }
    return new_void();
}

void* visit_while(Stmt* stmt)
{
    WhileStmt* whileStmt = (WhileStmt*)(stmt->realStmt);
    while (obj_likely(eval(whileStmt->condition))) {
        accept(EvaluateStmtVisitor, whileStmt->body);
    }

    return new_void();
}

static Object* fun_call(List* args, void* declaration, ExecutionEnvironment closure)
{
    FunStmt* funDecl = (FunStmt*)declaration;
    Token* tkn = NULL;
    Node *node = NULL, *valueWrapper = NULL;
    int i = 0;
    Object* value = NULL;
    ExecutionEnvironment *prevEnv = CurrentEnv, env = closure;
    env.enclosing = prevEnv;
    CurrentEnv = &env;

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
    CurrentEnv = prevEnv;
    return value;
}

void* visit_fun(Stmt* stmt)
{
    FunStmt* funStmt = (FunStmt*)(stmt->realStmt);
    Object* obj = NULL;
    Callable* call = (Callable*)alloc(sizeof(Callable));
    memset(call, 0, sizeof(Callable));
    call->call = fun_call;
    call->arity = funStmt->args->count;
    call->declaration = (void*)stmt;
    call->closure = *CurrentEnv;
    obj = obj_new(CALLABLE_L, call, sizeof(Callable));
    env_add_variable(CurrentEnv, funStmt->name.lexeme, obj);
    return new_void();
}

void* visit_return(Stmt* stmt)
{
    Object* value = new_void();
    ReturnStmt* returnStmt = (ReturnStmt*)(stmt->realStmt);
    if (returnStmt->value != NULL) {
        obj_destroy(value);
        value = eval(returnStmt->value);
    }

    return value;
}

// Object

void obj_destroy(Object* obj)
{
    Callable* callable = NULL;
    if (obj != NULL) {
        switch (obj->type) {
        case NIL_L:
        case BOOL_L:
        case NUMBER_L:
        case STRING_L:
        case ERROR_L:
        case VOID_L:
            fr(obj->value);
            fr(obj);
            break;
        case CALLABLE_L:
            callable = (Callable*)obj->value;
            //env_destroy(&callable->closure);
            fr(obj->value);
            fr(obj);
            break;
        default:
            runtime_error("Unknown Object to destroy", &obj, 0);
            break;
        }
    }
}

Object* obj_new(LiteralType type, void* value, int valueSize)
{
    Object* obj = alloc(sizeof(Object));
    obj->type = type;
    obj->value = value;
    obj->valueSize = valueSize;
    return obj;
}

int obj_likely(Object* obj)
{
    if (obj->type == NIL_L) {
        return 0;
    }

    if (obj->type == BOOL_L) {
        return strcmp((char*)obj->value, TRUE_KEY) == 0;
    }

    return 1;
}