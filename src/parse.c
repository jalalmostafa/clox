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

static int match(TokenType type, TokenType types[], int n, Node** node)
{
    int i = 0;
    for (i = 0; i < n; i++) {
        if (MATCH(type, types[i])) {
            (*node) = (*node)->next;
            return 1;
        }
    }
    return 0;
}

static void error(Node* node, const char* msg)
{
    const Token* token = (Token*)node->data;
    if (token->type == ENDOFFILE) {
        except(ERROR_AT_EOF, msg);
    } else {
        except(ERROR_AT_LINE, token->line, msg, token->lexeme);
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

static Expr* binary_production(Node** node, Expr* (*rule)(Node** t), TokenType matchTokens[], int n)
{
    Expr *expr = rule(node), *exprRight = NULL;
    const Token* tknPrev = NULL;
    while (match(((Token*)(*node)->data)->type, matchTokens, n, node)) {
        tknPrev = (*node)->prev->data;
        exprRight = rule(node);
        expr = new_expr(BINARY, new_binary(*tknPrev, expr, exprRight));
    }
    return expr;
}

static Expr* primary(Node** node)
{
    Expr* groupedExpr = NULL;
    Node** n = NULL;
    int valueSize = 0;
    const Token* tkn = (Token*)(*node)->data;
    double* doubleLiteral = NULL;

    if (MATCH(tkn->type, TRUE)) {
        (*node) = (*node)->next;
        valueSize = strlen(TRUE_KEY) + 1;
        return new_expr(LITERAL, (void*)new_literal(clone(TRUE_KEY, valueSize), BOOL_L, valueSize));
    }

    if (MATCH(tkn->type, FALSE)) {
        (*node) = (*node)->next;
        valueSize = strlen(FALSE_KEY) + 1;
        return new_expr(LITERAL, (void*)new_literal(clone(FALSE_KEY, valueSize), BOOL_L, valueSize));
    }

    if (MATCH(tkn->type, NIL)) {
        (*node) = (*node)->next;
        valueSize = strlen(NIL_KEY) + 10;
        return new_expr(LITERAL, (void*)new_literal(clone(NIL_KEY, valueSize), NIL_L, valueSize));
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
    error(*node, "Unknown Identifier");
    return NULL;
}

static Expr* unary(Node** node)
{
    Node* prev = NULL;
    Expr* rightExpr = NULL;
    const Token *tkn = (*node)->data, *tknPrev = NULL;
    TokenType unaryTokens[] = {
        MINUS,
        BANG
    };
    if (match(tkn->type, unaryTokens, 2, node)) {
        tknPrev = (Token*)(*node)->prev->data;
        rightExpr = unary(node);
        return new_expr(UNARY, (void*)new_unary(*tknPrev, rightExpr));
    }

    return primary(node);
}

static Expr* mutiplication(Node** node)
{
    TokenType multiplicationTokens[] = {
        SLASH,
        STAR
    };
    return binary_production(node, unary, multiplicationTokens, 2);
}

static Expr* addition(Node** node)
{
    TokenType additionTokens[] = {
        MINUS,
        PLUS
    };
    return binary_production(node, mutiplication, additionTokens, 2);
}

static Expr* comparison(Node** node)
{
    TokenType comparisonTokens[] = {
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL
    };
    return binary_production(node, addition, comparisonTokens, 4);
}

static Expr* equality(Node** node)
{
    TokenType equalityTokens[] = {
        BANG_EQUAL,
        EQUAL_EQUAL
    };
    return binary_production(node, comparison, equalityTokens, 2);
}

static Node** unterminated_statement(Node** node)
{
    return consume(node, SEMICOLON, "Expect ';' after value");
}

static Stmt* new_statement(StmtType type, Expr* expr)
{
    Stmt* stmt = (Stmt*)alloc(sizeof(Stmt));
    stmt->type = type;
    stmt->expr = expr;
    return stmt;
}

static Stmt* new_terminated_statement(Node** node, StmtType type)
{
    Expr* expr = equality(node);
    if (expr != NULL && (node = unterminated_statement(node)) != NULL) {
        return new_statement(type, expr);
    }

    return NULL;
}

static Stmt* print_statement(Node** node)
{
    return new_terminated_statement(node, STMT_PRINT);
}

static Stmt* expression_statement(Node** node)
{
    return new_terminated_statement(node, STMT_EXPR);
}

static Stmt* statement(Node** node)
{
    const Token* tkn = (Token*)((*node)->data);
    if (MATCH(tkn->type, PRINT)) {
        (*node) = (*node)->next;
        return print_statement(node);
    }

    return expression_statement(node);
}

static void expr_destroy(Expr* expr)
{
    Expr* ex = NULL;
    switch (expr->type) {
    case LITERAL:
        fr(((LiteralExpr*)expr->expr)->value);
        fr(((LiteralExpr*)expr->expr));
        break;
    case UNARY:
        ex = ((UnaryExpr*)expr->expr)->expr;
        expr_destroy(ex);
        break;
    case BINARY:
        ex = ((BinaryExpr*)expr->expr)->leftExpr;
        expr_destroy(ex);
        ex = ((BinaryExpr*)expr->expr)->rightExpr;
        expr_destroy(ex);
        break;
    case GROUPING:
        ex = ((GroupingExpr*)expr->expr)->expr;
        expr_destroy(ex);
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

static void stmts_destroy(List* stmts)
{
    list_foreach(stmts, expr_destroy);
    list_destroy(stmts);
}

void parser_destroy(ParsingContext* ctx)
{
    int i = 0;
    if (ctx == NULL || ctx->stmts == 0) {
        return;
    }
    stmts_destroy(ctx->stmts);
    ctx->stmts = NULL;
}

void* accept_expr(ExpressionVisitor visitor, Expr* expr)
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

void* accept(StmtVisitor visitor, Stmt* stmt)
{
    switch (stmt->type) {
    case STMT_PRINT:
        return visitor.visitPrintStmt(stmt);
    case STMT_EXPR:
        return visitor.visitExpressionStmt(stmt);
    }
    return NULL;
}

ParsingContext parse(Tokenization* toknz)
{
    ParsingContext ctx;
    List* stmts = NULL;
    List* tokens = toknz->values;
    int nbTokens = 0;
    Node* head = NULL;
    Stmt* stmt = NULL;

    memset(&ctx, 0, sizeof(ParsingContext));
    if (tokens != NULL) {
        stmts = list();
        nbTokens = tokens->count;
        head = tokens->head;

        while (!END_OF_TOKENS(((Expr*)head->data)->type)) {
            stmt = statement(&head);
            if (stmt != NULL) {
                list_push(stmts, stmt);
            } else {
                stmts_destroy(stmts);
                stmts = NULL;
                break;
            }
        }
    }
    ctx.stmts = stmts;
    return ctx;
}
