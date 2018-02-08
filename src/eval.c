#include "eval.h"
#include "global.h"
#include "mem.h"
#include "parse.h"
#include "tokenizer.h"
#include <stdio.h>
#include <string.h>

ExpressionVisitor EvalVisitor = {
    .visitLiteral = visit_literal,
    .visitGrouping = visit_grouping,
    .visitUnary = visit_unary,
    .visitBinary = visit_binary
};

static Object* eval(Expr* expr)
{
    return (Object*)accept(EvalVisitor, expr);
}

static char* expr_likely(LiteralExpr* expr)
{
    if (expr->type == NIL_L) {
        return FALSE_KEY;
    }

    if (expr->type == BOOL_L) {
        return strcmp(TRUE_KEY, (const char*)expr->value) == 0 ? TRUE_KEY : FALSE_KEY;
    }

    return TRUE_KEY;
}

static char* expr_unlikely(LiteralExpr* expr)
{
    if (expr->type == NIL_L) {
        return TRUE_KEY;
    }

    if (expr->type == BOOL_L) {
        return strcmp(TRUE_KEY, (const char*)expr->value) != 0 ? TRUE_KEY : FALSE_KEY;
    }

    return FALSE_KEY;
}

static char* likely(int condition)
{
    if (condition > 0) {
        return TRUE_KEY;
    }
    return FALSE_KEY;
}

static Object* runtime_error(const char* format, Object* obj, int line)
{
    int len = 0;
    char buffer[LINEBUFSIZE];
    memset(buffer, 0, LINEBUFSIZE);
    snprintf(buffer, LINEBUFSIZE, format, line);
    len = strlen(buffer) + 1;
    char* msg = clone(buffer, len);
    obj->type = ERROR_L;
    obj->value = msg;
    return obj;
}

void* visit_binary(void* expr)
{
    const BinaryExpr* bexpr = (BinaryExpr*)expr;
    Object* rObject = eval(bexpr->rightExpr);
    Object* lObject = eval(bexpr->leftExpr);
    Object* result = NULL;
    double *value = NULL, *lvalue = NULL, *rvalue = NULL;
    char* svalue = NULL;
    int valueLengthRight = 0, valueLengthLeft = 0, ordinary = 0;

    if (rObject == NULL || lObject == NULL) {
        return NULL;
    }
    result = (Object*)alloc(sizeof(Object));
    memset(result, 0, sizeof(Object));
    switch (bexpr->op.type) {
    case MINUS:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = NUMBER_L;
            result->value = alloc(sizeof(double));
            value = (double*)result->value;
            *value = *((double*)lObject->value) - *((double*)rObject->value);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case PLUS:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = NUMBER_L;
            result->value = alloc(sizeof(double));
            value = (double*)result->value;
            *value = *((double*)lObject->value) + *((double*)rObject->value);
        } else if (rObject->type == STRING_L && lObject->type == STRING_L) {
            valueLengthLeft = strlen((char*)lObject->value);
            valueLengthRight = strlen((char*)rObject->value);
            result->type = STRING_L;
            svalue = alloc(valueLengthLeft + valueLengthRight + 1);
            memcpy(svalue, lObject->value, valueLengthLeft);
            memcpy(svalue + valueLengthLeft, rObject->value, valueLengthRight + 1);
            result->value = svalue;
        } else {
            runtime_error(OPERAND_SAMETYPE, result, bexpr->op.line);
        }
        break;
    case SLASH:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = NUMBER_L;
            result->value = alloc(sizeof(double));
            value = (double*)result->value;
            *value = *((double*)lObject->value) / *((double*)rObject->value);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case STAR:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = NUMBER_L;
            result->value = alloc(sizeof(double));
            value = (double*)result->value;
            *value = *((double*)lObject->value) * *((double*)rObject->value);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case GREATER:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = BOOL_L;
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            ordinary = (*lvalue > *rvalue);
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case GREATER_EQUAL:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = BOOL_L;
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            ordinary = (*lvalue >= *rvalue);
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case LESS:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = BOOL_L;
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            ordinary = (*lvalue < *rvalue);
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case LESS_EQUAL:
        if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = BOOL_L;
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            ordinary = (*lvalue <= *rvalue);
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else {
            runtime_error(OPERAND_NUMBER, result, bexpr->op.line);
        }
        break;
    case EQUAL_EQUAL:
        result->type = BOOL_L;
        if (rObject->type == NIL_L && lObject->type == NIL_L) {
            result->value = clone(likely(1), strlen(likely(1)) + 1);
        } else if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            ordinary = (*lvalue == *rvalue);
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else if ((rObject->type == STRING_L && lObject->type == STRING_L)
            || (rObject->type == BOOL_L && lObject->type == BOOL_L)) {
            ordinary = strcmp(rObject->value, lObject->value) == 0;
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else {
            result->value = clone(likely(0), strlen(likely(0)) + 1);
        }
        break;
    case BANG_EQUAL:
        result->type = BOOL_L;
        if (rObject->type == NIL_L && lObject->type == NIL_L) {
            result->value = clone(likely(0), strlen(likely(0)) + 1);
        } else if (rObject->type == NUMBER_L && lObject->type == NUMBER_L) {
            result->type = BOOL_L;
            lvalue = (double*)lObject->value;
            rvalue = (double*)rObject->value;
            ordinary = (*lvalue != *rvalue);
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else if ((rObject->type == STRING_L && lObject->type == STRING_L)
            || (rObject->type == BOOL_L && lObject->type == BOOL_L)) {
            ordinary = strcmp(rObject->value, lObject->value) != 0;
            result->value = clone(likely(ordinary), strlen(likely(ordinary)) + 1);
        } else {
            result->value = clone(likely(1), strlen(likely(1)) + 1);
        }
        break;
    default:
        break;
    }
    fr(rObject->value);
    fr(rObject);
    fr(lObject->value);
    fr(lObject);
    return result;
}

void* visit_unary(void* expr)
{
    const UnaryExpr* uexpr = (UnaryExpr*)expr;
    Object* rObject = eval(uexpr->expr);
    char* st = NULL;
    double* value = NULL;
    switch (uexpr->op.type) {
    case BANG:
        st = expr_unlikely(rObject);
        fr(rObject->value);
        rObject->type = BOOL_L;
        rObject->value = clone(st, strlen(st) + 1);
        break;
    case MINUS:
        if (rObject->type != NUMBER_L) {
            runtime_error(OPERAND_NUMBER, rObject, uexpr->op.line);
        } else {
            value = clone((double*)rObject->value, sizeof(double));
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
    LiteralExpr* lexpr = (LiteralExpr*)clone(original, sizeof(LiteralExpr));
    lexpr->value = clone(original->value, original->valueSize);
    return lexpr;
}