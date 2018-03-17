#include "parse.h"
#include "ds/list.h"
#include "except.h"
#include "global.h"
#include "mem.h"
#include "tokenizer.h"
#include <stdio.h>
#include <string.h>

static Expr* expression(Node** node);
static Expr* assignment(Node** node);
static Expr* equality(Node** node);
static Expr* comparison(Node** node);
static Expr* addition(Node** node);
static Expr* mutiplication(Node** node);
static Expr* unary(Node** node);
static Expr* primary(Node** node);

static Stmt* block_statements(Node** node);

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
        return &(*node)->prev;
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

static UnaryExpr* new_unary(Token op, Expr* internalExpr)
{
    UnaryExpr* expr = (UnaryExpr*)alloc(sizeof(UnaryExpr));
    expr->op = op;
    expr->expr = internalExpr;
    return expr;
}

static BinaryExpr* new_binary(Token op, Expr* left, Expr* right)
{
    BinaryExpr* expr = (BinaryExpr*)alloc(sizeof(BinaryExpr));
    expr->leftExpr = (Expr*)left;
    expr->rightExpr = (Expr*)right;
    expr->op = op;
    return expr;
}

static GroupingExpr* new_grouping(Expr* internalExpr)
{
    GroupingExpr* expr = (GroupingExpr*)alloc(sizeof(GroupingExpr));
    expr->expr = (Expr*)internalExpr;
    return expr;
}

static VariableExpr* new_variable(Token variableName)
{
    VariableExpr* expr = (VariableExpr*)alloc(sizeof(VariableExpr));
    expr->variableName = variableName;
    return expr;
}

static AssignmentExpr* new_assignment(Token variableName, Expr* rightExpr)
{
    AssignmentExpr* expr = (AssignmentExpr*)alloc(sizeof(AssignmentExpr));
    expr->rightExpr = rightExpr;
    expr->variableName = variableName;
    return expr;
}

static Expr* binary_production(Node** node, Expr* (*rule)(Node** t), TokenType matchTokens[], int n)
{
    Expr *expr = rule(node), *exprRight = NULL;
    const Token* tknPrev = NULL;
    while (match(((Token*)(*node)->data)->type, matchTokens, n, node)) {
        tknPrev = (Token*)(*node)->prev->data;
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
        return new_expr(LITERAL, (void*)new_literal(clone((void*)TRUE_KEY, valueSize), BOOL_L, valueSize));
    }

    if (MATCH(tkn->type, FALSE)) {
        (*node) = (*node)->next;
        valueSize = strlen(FALSE_KEY) + 1;
        return new_expr(LITERAL, (void*)new_literal(clone((void*)FALSE_KEY, valueSize), BOOL_L, valueSize));
    }

    if (MATCH(tkn->type, NIL)) {
        (*node) = (*node)->next;
        valueSize = strlen(NIL_KEY) + 1;
        return new_expr(LITERAL, (void*)new_literal(clone((void*)NIL_KEY, valueSize), NIL_L, valueSize));
    }

    if (MATCH(tkn->type, STRING)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, new_literal(clone(tkn->literal, strlen(tkn->literal) + 1), STRING_L, strlen(tkn->literal) + 1));
    }

    if (MATCH(tkn->type, NUMBER)) {
        (*node) = (*node)->next;
        doubleLiteral = (double*)alloc(sizeof(double));
        *doubleLiteral = atof(tkn->literal);
        return new_expr(LITERAL, new_literal(doubleLiteral, NUMBER_L, sizeof(double)));
    }

    if (MATCH(tkn->type, LEFT_PAREN)) {
        *node = (*node)->next;
        groupedExpr = expression(node);
        n = consume(node, RIGHT_PAREN, "Expect ')' after expression.");
        if (n == NULL) {
            return NULL;
        }
        return new_expr(GROUPING, (void*)new_grouping(groupedExpr));
    }

    if (MATCH(tkn->type, IDENTIFIER)) {
        *node = (*node)->next;
        return new_expr(VARIABLE, new_variable(*(Token*)(*node)->prev->data));
    }
    error(*node, UNKNOWN_IDENTIFIER);
    return NULL;
}

static Expr* unary(Node** node)
{
    Expr* rightExpr = NULL;
    const Token *tkn = (Token*)(*node)->data, *tknPrev = NULL;
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

static Expr* assignment(Node** node)
{
    Expr *expr = equality(node), *nextExpr = NULL;
    Node* equals = *node;
    if (MATCH(((Token*)equals->data)->type, EQUAL)) {
        (*node) = (*node)->next;
        nextExpr = assignment(node);
        if (expr != NULL && expr->type == VARIABLE) {
            return new_expr(ASSIGNMENT, new_assignment(((VariableExpr*)expr->expr)->variableName, nextExpr));
        }
        error(equals, "Invalid Assignment Target");
    }

    return expr;
}

static Expr* expression(Node** node)
{
    return assignment(node);
}

static void synchronize(Node** node)
{
    (*node) = (*node)->next;
    if (*node != NULL) {

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
}

static Node** terminated_statement(Node** node)
{
    return consume(node, SEMICOLON, "Expect ';' after value");
}

static Stmt* new_statement(StmtType type, void* realStmt)
{
    Stmt* stmt = (Stmt*)alloc(sizeof(Stmt));
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = type;
    stmt->realStmt = realStmt;
    return stmt;
}

static Stmt* new_terminated_statement(Node** node, StmtType type, void* realStmt)
{
    if (terminated_statement(node) != NULL) {
        return new_statement(type, realStmt);
    }

    return NULL;
}

static Stmt* print_statement(Node** node)
{
    Expr* expr = expression(node);
    PrintStmt* stmt = (PrintStmt*)alloc(sizeof(PrintStmt));
    stmt->expr = expr;
    return new_terminated_statement(node, STMT_PRINT, stmt);
}

static Stmt* expression_statement(Node** node)
{
    Expr* expr = expression(node);
    ExprStmt* stmt = (ExprStmt*)alloc(sizeof(ExprStmt));
    stmt->expr = expr;
    return new_terminated_statement(node, STMT_EXPR, stmt);
}

static Stmt* var_statement(Node** node, Expr* initializer, Token variableName)
{
    VarDeclarationStmt* stmt = (VarDeclarationStmt*)alloc(sizeof(VarDeclarationStmt));
    stmt->initializer = initializer;
    stmt->varName = variableName;
    return new_terminated_statement(node, STMT_VAR_DECLARATION, stmt);
}

static Stmt* var_declaration(Node** node)
{
    Node** identifierNode = consume(node, IDENTIFIER, "Expected a variable name");
    Token* name = NULL;
    Expr* initializer = NULL;

    if (identifierNode == NULL) {
        return NULL;
    }
    name = (Token*)(*identifierNode)->data;

    if (MATCH(((Token*)(*node)->data)->type, EQUAL)) {
        (*node) = (*node)->next;
        initializer = expression(node);
    }

    return var_statement(node, initializer, *name);
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

static Stmt* declaration(Node** node)
{
    const Token* tkn = (Token*)((*node)->data);
    Stmt* stmt = NULL;
    if (MATCH(tkn->type, VAR)) {
        (*node) = (*node)->next;
        stmt = var_declaration(node);
    } else if (MATCH(tkn->type, LEFT_BRACE)) {
        (*node) = (*node)->next;
        stmt = block_statements(node);
    } else {
        stmt = statement(node);
    }
    if (stmt == NULL) {
        synchronize(node);
    }

    return stmt;
}

static Stmt* block_statements(Node** node)
{
    Token* token = NULL;
    BlockStmt* stmt = (BlockStmt*)alloc(sizeof(BlockStmt));
    stmt->innerStmts = list();
    token = (Token*)(*node)->data;
    while (token->type != RIGHT_BRACE && token->type != ENDOFFILE) {
        list_push(stmt->innerStmts, declaration(node));
        token = (Token*)(*node)->data;
    }
    consume(node, RIGHT_BRACE, "Expect '}' after block.");
    return new_statement(STMT_BLOCK, (void*)stmt);
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
    case ASSIGNMENT:
        ex = ((AssignmentExpr*)expr->expr)->rightExpr;
        expr_destroy(ex);
    case VARIABLE:
    default:
        break;
    }
    fr(expr);
}

void stmt_destroy(void* stmtObj)
{
    Stmt* stmt = (Stmt*)stmtObj;
    switch (stmt->type) {
    case STMT_BLOCK:
        list_foreach(((BlockStmt*)stmt->realStmt)->innerStmts, stmt_destroy);
        break;
    case STMT_PRINT:
        expr_destroy(((PrintStmt*)stmt->realStmt)->expr);
        break;
    case STMT_EXPR:
        expr_destroy(((ExprStmt*)stmt->realStmt)->expr);
        break;
    case STMT_VAR_DECLARATION:
        expr_destroy(((VarDeclarationStmt*)stmt->realStmt)->initializer);
        break;
    }
    fr((void*)stmt);
}

static void stmts_destroy(List* stmts)
{
    if (stmts->count != 0) {
        list_foreach(stmts, stmt_destroy);
    }
    list_destroy(stmts);
}

void parser_destroy(ParsingContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    if (ctx->stmts != NULL) {
        stmts_destroy(ctx->stmts);
        ctx->stmts = NULL;
    }
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
    case VARIABLE:
        return visitor.visitVariableExpr(expr->expr);
    case ASSIGNMENT:
        return visitor.visitAssignmentExpr(expr->expr);
    }
    return NULL;
}

void* accept(StmtVisitor visitor, Stmt* stmt)
{
    switch (stmt->type) {
    case STMT_PRINT:
        return visitor.visitPrintStmt(stmt->realStmt);
    case STMT_EXPR:
        return visitor.visitExpressionStmt(stmt->realStmt);
    case STMT_VAR_DECLARATION:
        return visitor.visitVarDeclarationStmt(stmt->realStmt);
    case STMT_BLOCK:
        return visitor.visitBlock(stmt->realStmt);
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

        while (!END_OF_TOKENS(((Token*)head->data)->type)) {
            stmt = declaration(&head);
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
