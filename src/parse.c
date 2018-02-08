#include "parse.h"
#include "ds/list.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include "tokenizer.h"
#include <stdio.h>
#include <string.h>

static Expr* equality(Node** token);
static Expr* comparison(Node** token);
static Expr* addition(Node** token);
static Expr* mutiplication(Node** token);
static Expr* unary(Node** token);
static Expr* primary(Node** token);

static int match(TokenType type, TokenType types[], int n, Node** token)
{
    int i = 0;
    for (i = 0; i < n; i++) {
        if (MATCH(type, types[i])) {
            (*token) = (*token)->next;
            return 1;
        }
    }
    return 0;
}

static void error(Node* node, const char* msg)
{
    const Token* token = (Token*)node->data;
    if (token->type == EOF) {
        except("Syntax Error at end of file: %s", msg);
    } else {
        except("Syntax Error (Line %d): %s '%s'", token->line, msg, token->lexeme);
    }
}

static Node** consume(Node** node, TokenType type, const char* msg)
{
    const Token* tkn = (Token*)(*node)->data;
    if (MATCH(tkn->type, type)) {
        (*node) = (*node)->next;
        return node;
    }
    error(*node, msg);
    return NULL;
}

static Expr* new_expr(ExpressionType type, void* realExpr)
{
    Expr* expr = (Expr*)alloc(sizeof(Expr));
    expr->expr = realExpr;
    expr->type = type;
    return expr;
}

static LiteralExpr* new_literal(void* value, LiteralType type, int size)
{
    LiteralExpr* expr = (LiteralExpr*)alloc(sizeof(LiteralExpr));
    expr->value = value;
    expr->type = type;
    expr->valueSize = size;
    return expr;
}

static UnaryExpr* new_unary(Token op, void* internalExpr)
{
    UnaryExpr* expr = (UnaryExpr*)alloc(sizeof(UnaryExpr));
    expr->op = op;
    expr->expr = internalExpr;
    return expr;
}

static BinaryExpr* new_binary(Token op, void* left, void* right)
{
    BinaryExpr* expr = (BinaryExpr*)alloc(sizeof(BinaryExpr));
    expr->leftExpr = left;
    expr->rightExpr = right;
    expr->op = op;
    return expr;
}

static GroupingExpr* new_grouping(void* internalExpr)
{
    GroupingExpr* expr = (GroupingExpr*)alloc(sizeof(GroupingExpr));
    expr->expr = internalExpr;
    return expr;
}

static Expr* binary_production(Node** token, Expr* (*rule)(Node** t), TokenType matchTokens[], int n)
{
    Expr *expr = rule(token), *exprRight = NULL;
    const Token* tknPrev = NULL;
    while (match(((Token*)(*token)->data)->type, matchTokens, n, token)) {
        tknPrev = (*token)->prev->data;
        exprRight = rule(token);
        expr = new_expr(BINARY, new_binary(*tknPrev, expr, exprRight));
    }
    return expr;
}

static Expr* primary(Node** node)
{
    Expr* groupedExpr = NULL;
    Node** n = NULL;
    const Token* tkn = (Token*)(*node)->data;
    double* doubleLiteral = NULL;

    if (MATCH(tkn->type, TRUE)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, (void*)new_literal(TRUE_KEY, BOOL_L, strlen(TRUE_KEY) + 1));
    }

    if (MATCH(tkn->type, FALSE)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, (void*)new_literal(FALSE_KEY, BOOL_L, strlen(FALSE_KEY) + 1));
    }

    if (MATCH(tkn->type, NIL)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, (void*)new_literal(NIL_KEY, NIL_L, strlen(NIL_KEY) + 1));
    }

    if (MATCH(tkn->type, STRING)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, new_literal(tkn->literal, STRING_L, strlen(tkn->literal) + 1));
    }

    if (MATCH(tkn->type, NUMBER)) {
        (*node) = (*node)->next;
        doubleLiteral = (double*)alloc(sizeof(double));
        *doubleLiteral = atof(tkn->literal);
        return new_expr(LITERAL, new_literal(doubleLiteral, NUMBER_L, sizeof(double)));
    }

    if (MATCH(tkn->type, LEFT_PAREN)) {
        *node = (*node)->next;
        groupedExpr = equality(node);
        n = consume(node, RIGHT_PAREN, "Expect ')' after expression.");
        if (n == NULL) {
            return NULL;
        }
        return new_expr(GROUPING, (void*)new_grouping(groupedExpr));
    }
    except("Unexpected identifier");
    return NULL;
}

static Expr* unary(Node** token)
{
    Node* prev = NULL;
    Expr* rightExpr = NULL;
    const Token *tkn = (*token)->data, *tknPrev = NULL;
    TokenType unaryTokens[] = {
        MINUS,
        BANG
    };
    if (match(tkn->type, unaryTokens, 2, token)) {
        tknPrev = (Token*)(*token)->prev->data;
        rightExpr = unary(token);
        return new_expr(UNARY, (void*)new_unary(*tknPrev, rightExpr));
    }
    return primary(token);
}

static Expr* mutiplication(Node** token)
{
    TokenType multiplicationTokens[] = {
        SLASH,
        STAR
    };
    return binary_production(token, unary, multiplicationTokens, 2);
}

static Expr* addition(Node** token)
{
    TokenType additionTokens[] = {
        MINUS,
        PLUS
    };
    return binary_production(token, mutiplication, additionTokens, 2);
}

static Expr* comparison(Node** token)
{
    TokenType comparisonTokens[] = {
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL
    };
    return binary_production(token, addition, comparisonTokens, 4);
}

static Expr* equality(Node** token)
{
    TokenType equalityTokens[] = {
        BANG_EQUAL,
        EQUAL_EQUAL
    };
    return binary_production(token, comparison, equalityTokens, 2);
}

Expr* parse(Tokenization* toknz)
{
    List* tokens = toknz->values;
    int nbTokens = 0, i = 0;
    Node* head = NULL;

    if (tokens == NULL) {
        return NULL;
    }
    nbTokens = tokens->count;
    head = tokens->head;

    return equality(&head);
}

void destroy_expr(Expr* expr)
{
    void* ex = NULL;
    switch (expr->type) {
    case LITERAL:
        fr(((LiteralExpr*)expr->expr)->value);
        fr(((LiteralExpr*)expr->expr));
        break;
    case UNARY:
        ex = (UnaryExpr*)(expr->expr);
        destroy_expr(ex);
        fr(ex);
        break;
    case BINARY:
        ex = ((BinaryExpr*)expr->expr)->leftExpr;
        destroy_expr(ex);
        ex = ((BinaryExpr*)expr->expr)->rightExpr;
        destroy_expr(ex);
        break;
    case GROUPING:
        ex = ((GroupingExpr*)expr)->expr;
        destroy_expr(ex);
        break;
    default:
        break;
    }
    fr(expr);
}

static void synchronize(Node** node)
{
    (*node) = (*node)->next;
    const Token *token = (Token*)(*node)->data, *prevToken;
    while (!END_OF_TOKENS(token->type)) {
        prevToken = (Token*)(*node)->prev->data;
        if (prevToken->type == SEMICOLON)
            return;

        switch (token->type) {
        case CLASS:
        case FUN:
        case VAR:
        case FOR:
        case IF:
        case WHILE:
        case PRINT:
        case RETURN:
            return;
        }
        (*node) = (*node)->next;
    }
}

void* accept(ExpressionVisitor visitor, Expr* expr)
{
    switch (expr->type) {
    case LITERAL:
        return visitor.visitLiteral(expr->expr);
    case UNARY:
        return visitor.visitUnary(expr->expr);
    case BINARY:
        return visitor.visitBinary(expr->expr);
    case GROUPING:
        return visitor.visitGrouping(expr->expr);
    }
    return NULL;
}