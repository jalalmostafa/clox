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

ExpressionVisitor EvaluateExpressionVisitor = {
    visit_binary,
    visit_unary,
    visit_literal,
    visit_grouping,
    visit_var_expr,
    visit_assign
};

StmtVisitor EvaluateStmtVistior = {
    visit_print,
    visit_var,
    visit_expr,
    visit_block,
    visit_ifElse,
    visit_for,
    visit_while
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

static int obj_likely(Object* obj)
{
    if (obj->type == NIL_L) {
        return 0;
    }

    if (obj->type == BOOL_L) {
        return strcmp((char*)obj->value, TRUE_KEY) == 0;
    }

    return 1;
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
    result = (Object*)alloc(sizeof(Object));
    memset(result, 0, sizeof(Object));
    result->type = BOOL_L;
    result->value = clone((void*)likely(truthy), strlen(likely(truthy)) + 1);
    result->valueSize = strlen(likely(truthy)) + 1;
    return result;
}

void* visit_binary(void* expr)
{
    const BinaryExpr* bexpr = (BinaryExpr*)expr;
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
        result->type = BOOL_L;
        if (rObject->type == NIL_L && lObject->type == NIL_L) {
            result->value = clone((void*)likely(1), strlen(likely(1)) + 1);
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
    default:
        break;
    }

    fr(lObject->value);
    fr(lObject);
    fr(rObject->value);
    fr(rObject);
    return result;
}

void* visit_unary(void* expr)
{
    const UnaryExpr* uexpr = (UnaryExpr*)expr;
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

void* visit_grouping(void* expr)
{
    const GroupingExpr* gexpr = (GroupingExpr*)expr;
    return eval(gexpr->expr);
}

void* visit_literal(void* expr)
{
    LiteralExpr* original = (LiteralExpr*)expr;
    Object* result = (Object*)clone(original, sizeof(LiteralExpr));
    result->value = clone(original->value, original->valueSize);
    return result;
}

void* visit_var_expr(void* exprObject)
{
    VariableExpr* expr = (VariableExpr*)exprObject;
    Object* value = env_get_variable_value(CurrentEnv, expr->variableName.lexeme);
    if (value == NULL) {
        runtime_error("Unresolved variable name '%s'", &value, expr->variableName.line, expr->variableName.lexeme);
    }
    return value;
}

void* visit_print(void* stmtObj)
{
    PrintStmt* stmt = (PrintStmt*)stmtObj;
    Object* obj = eval(stmt->expr);
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
    }
    return new_void();
}

void* visit_expr(void* stmtObj)
{
    ExprStmt* stmt = (ExprStmt*)stmtObj;
    return eval(stmt->expr);
}

void* visit_var(void* stmtObj)
{
    VarDeclarationStmt* stmt = (VarDeclarationStmt*)stmtObj;
    Object* value = NULL;
    Token key = stmt->varName;
    if (stmt->initializer != NULL) {
        value = eval(stmt->initializer);
    }
    if (!env_add_variable(CurrentEnv, key.lexeme, value)) {
        runtime_error("'%s' is already defined", &value, key.line, key.lexeme);
    }

    return value;
}

void* visit_assign(void* exprObj)
{
    AssignmentExpr* expr = (AssignmentExpr*)exprObj;
    Object* value = eval(expr->rightExpr);

    if (value == NULL) {
        runtime_error("Cannot assign undeclared variable '%s'", &value, expr->variableName.line, expr->variableName.lexeme);
    } else {
        env_set_variable_value(CurrentEnv, expr->variableName.lexeme, value);
    }

    return value;
}

void* visit_block(void* blockObj)
{
    BlockStmt* stmt = (BlockStmt*)blockObj;
    Stmt* innerStmt = NULL;
    Node* node = NULL;
    ExecutionEnvironment *prevEnv = CurrentEnv, env = { NULL, prevEnv };
    CurrentEnv = &env;
    for (node = stmt->innerStmts->head; node != NULL; node = node->next) {
        innerStmt = (Stmt*)node->data;
        accept(EvaluateStmtVistior, innerStmt);
    }
    env_destroy(CurrentEnv);
    CurrentEnv = prevEnv;
    return new_void();
}

void* visit_ifElse(void* ifElseObj)
{
    IfElseStmt* stmt = (IfElseStmt*)ifElseObj;
    Object* evalCond = eval(stmt->condition);
    if (obj_likely(evalCond)) {
        return accept(EvaluateStmtVistior, stmt->thenStmt);
    } else if (stmt->elseStmt != NULL) {
        return accept(EvaluateStmtVistior, stmt->elseStmt);
    }
    return new_void();
}

void* visit_while(void* whileObj)
{
    WhileStmt* stmt = (WhileStmt*)whileObj;
    while (obj_likely(eval(stmt->condition))) {
        accept(EvaluateStmtVistior, stmt->body);
    }

    return new_void();
}

void* visit_for(void* forObj)
{
    return new_void();
}