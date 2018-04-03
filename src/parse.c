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
static Expr* call(Node** node);
static Expr* primary(Node** node);
static Expr* logicOr(Node** node);
static Expr* logicAnd(Node** node);

static Stmt* block_statements(Node** node);
static Stmt* if_statement(Node** node);
static Stmt* for_statement(Node** node);
static Stmt* while_statement(Node** node);
static Stmt* fun_statement(const char* type, Node** node);

static void expr_destroy(Expr* expr);

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

LiteralExpr* new_true()
{
    int valueSize = strlen(TRUE_KEY) + 1;
    return new_literal(clone((void*)TRUE_KEY, valueSize), BOOL_L, valueSize);
}

LiteralExpr* new_false()
{
    int valueSize = strlen(FALSE_KEY) + 1;
    return new_literal(clone((void*)TRUE_KEY, valueSize), BOOL_L, valueSize);
}

LiteralExpr* new_nil()
{
    int valueSize = strlen(NIL_KEY) + 1;
    return new_literal(clone((void*)NIL_KEY, valueSize), NIL_L, valueSize);
}

static Expr* primary(Node** node)
{
    Expr* groupedExpr = NULL;
    Node** n = NULL;
    const Token* tkn = (Token*)(*node)->data;
    double* doubleLiteral = NULL;

    if (MATCH(tkn->type, TRUE)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, (void*)new_true());
    }

    if (MATCH(tkn->type, FALSE)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, (void*)new_false());
    }

    if (MATCH(tkn->type, NIL)) {
        (*node) = (*node)->next;
        return new_expr(LITERAL, (void*)new_nil());
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

static CallExpr* new_call(Expr* callee, List* args, Token paren)
{
    CallExpr* call = alloc(sizeof(CallExpr));
    call->callee = callee;
    call->paren = paren;
    call->args = args;
    return call;
}

static Expr* finish_call(Node** node, Expr* callee)
{
    Token *tkn = NULL, *paren = NULL;
    List* args = list();
    Expr* arg = NULL;
    Node** temp = NULL;
    do {
        (*node) = (*node)->next;
        if (args->count > MAX_ARGS) {
            error(*node, "Cannot have more than %d args");
            list_destroy(args);
            expr_destroy(callee);
            return NULL;
        }
        tkn = (Token*)(*node)->data;
        if (!MATCH(tkn->type, RIGHT_PAREN)) {
            arg = expression(node);
        }
        if (arg != NULL) {
            list_push(args, arg);
        }
        tkn = (Token*)(*node)->data;
    } while (MATCH(tkn->type, COMMA));
    temp = consume(node, RIGHT_PAREN, "Expect ')' for function call");
    paren = (Token*)(*temp)->data;
    return new_expr(CALL, new_call(callee, args, *paren));
}

static Expr* call(Node** node)
{
    Expr* expr = primary(node);
    Token* tkn = (Token*)(*node)->data;
    while (1) {
        if (MATCH(tkn->type, LEFT_PAREN)) {
            expr = finish_call(node, expr);
            tkn = (Token*)(*node)->data;
        } else {
            break;
        }
    }
    return expr;
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

    return call(node);
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
    Expr *expr = logicOr(node), *nextExpr = NULL;
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

static Expr* new_logical(Expr* left, Token op, Expr* right)
{
    LogicalExpr* logicalExpr = (LogicalExpr*)alloc(sizeof(LogicalExpr));
    logicalExpr->op = op;
    logicalExpr->left = left;
    logicalExpr->right = right;
    return new_expr(LOGICAL, logicalExpr);
}

static Expr* logicOr(Node** node)
{
    Expr *expr = logicAnd(node), *right = NULL;
    const Token* tkn = (Token*)(*node)->data;
    Token* operatorTkn = NULL;

    while (MATCH(tkn->type, OR)) {
        operatorTkn = (Token*)(*node)->data;
        (*node) = (*node)->next;
        tkn = (Token*)(*node)->data;
        right = logicAnd(node);
        expr = new_logical(expr, *operatorTkn, right);
    }
    return expr;
}

static Expr* logicAnd(Node** node)
{
    Expr *expr = equality(node), *right = NULL;
    Token *tkn = (Token*)(*node)->data, *operatorTkn = NULL;

    while (MATCH(tkn->type, AND)) {
        operatorTkn = (Token*)(*node)->data;
        (*node) = (*node)->next;
        tkn = (Token*)(*node)->data;
        right = equality(node);
        expr = new_logical(expr, *operatorTkn, right);
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
    } else if (MATCH(tkn->type, LEFT_BRACE)) {
        (*node) = (*node)->next;
        return block_statements(node);
    } else if (MATCH(tkn->type, IF)) {
        (*node) = (*node)->next;
        return if_statement(node);
    } else if (MATCH(tkn->type, FOR)) {
        (*node) = (*node)->next;
        return for_statement(node);
    } else if (MATCH(tkn->type, WHILE)) {
        (*node) = (*node)->next;
        return while_statement(node);
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
    } else if (MATCH(tkn->type, FUN)) {
        (*node) = (*node)->next;
        return fun_statement("function", node);
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

static Stmt* if_statement(Node** node)
{
    Stmt *thenStmt = NULL, *elseStmt = NULL;
    IfElseStmt* realStmt = NULL;
    Token* tkn = (Token*)(*node)->data;
    Expr* condition = NULL;
    consume(node, LEFT_PAREN, "Expect '(' after 'if'.");
    condition = expression(node);
    consume(node, RIGHT_PAREN, "Expect ')' after condition.");
    thenStmt = statement(node);
    tkn = (Token*)(*node)->data;
    if (MATCH(tkn->type, ELSE)) {
        (*node) = (*node)->next;
        elseStmt = statement(node);
    }
    realStmt = (IfElseStmt*)alloc(sizeof(IfElseStmt));
    realStmt->condition = condition;
    realStmt->elseStmt = elseStmt;
    realStmt->thenStmt = thenStmt;
    return new_statement(STMT_IF_ELSE, realStmt);
}

static Stmt* for_statement(Node** node)
{
    consume(node, LEFT_PAREN, "Expect '(' after for");
    Token* tkn = (Token*)(*node)->data;
    Stmt *initializer = NULL, *body = NULL;
    Expr *condition = NULL, *step = NULL;
    BlockStmt *wrappedBody = NULL, *wrappedForAndInit = NULL;
    WhileStmt* wrappedFor = NULL;
    ExprStmt* wrappedStep = NULL;
    if (MATCH(tkn->type, SEMICOLON)) {
        (*node) = (*node)->next;
    } else {
        if (MATCH(tkn->type, VAR)) {
            (*node) = (*node)->next;
            initializer = var_declaration(node);
        } else {
            (*node) = (*node)->next;
            initializer = expression_statement(node);
        }
    }
    tkn = (Token*)(*node)->data;
    if (!MATCH(tkn->type, SEMICOLON)) {
        condition = expression(node);
    }
    consume(node, SEMICOLON, "Expect ';' after for condition");
    if (!MATCH(tkn->type, RIGHT_PAREN)) {
        step = expression(node);
    }
    consume(node, RIGHT_PAREN, "Expect ')' for 'for' closing");
    body = statement(node);
    if (step != NULL) {
        wrappedBody = alloc(sizeof(BlockStmt));
        wrappedBody->innerStmts = list();
        wrappedStep = alloc(sizeof(ExprStmt));
        wrappedStep->expr = step;
        list_push(wrappedBody->innerStmts, body);
        list_push(wrappedBody->innerStmts, new_statement(STMT_EXPR, wrappedStep));
        body = new_statement(STMT_BLOCK, wrappedBody);
    }

    if (condition == NULL) {
        condition = new_expr(LITERAL, new_true());
    }
    wrappedFor = alloc(sizeof(WhileStmt));
    wrappedFor->condition = condition;
    wrappedFor->body = body;
    body = new_statement(STMT_WHILE, wrappedFor);
    if (initializer != NULL) {
        wrappedForAndInit = alloc(sizeof(BlockStmt));
        wrappedForAndInit->innerStmts = list();
        list_push(wrappedForAndInit->innerStmts, initializer);
        list_push(wrappedForAndInit->innerStmts, body);
        body = new_statement(STMT_BLOCK, wrappedForAndInit);
    }
    return body;
}

static Stmt* while_statement(Node** node)
{
    WhileStmt* realStmt = NULL;
    Expr* condition = NULL;
    Stmt* bodyStmt = NULL;
    consume(node, LEFT_PAREN, "Expect '(' after 'while'.");
    condition = expression(node);
    consume(node, RIGHT_PAREN, "Expect ')' after condition.");
    bodyStmt = statement(node);
    realStmt = (WhileStmt*)alloc(sizeof(WhileStmt));
    realStmt->condition = condition;
    realStmt->body = bodyStmt;
    return new_statement(STMT_WHILE, realStmt);
}

static Stmt* fun_statement(const char* kind, Node** node)
{
    Token *name = NULL, *tkn = NULL;
    Node** temp = NULL;
    List* params = NULL;
    Stmt* body = NULL;
    FunStmt* fnStmt = NULL;
    char buf[LINEBUFSIZE];
    memset(buf, 0, LINEBUFSIZE);
    sprintf(buf, "Expect %s name.", kind);
    temp = consume(node, IDENTIFIER, buf);
    name = (Token*)(*temp)->data;
    if (temp == NULL) {
        // do something about misssing fun name
    }
    memset(buf, 0, LINEBUFSIZE);
    sprintf(buf, "Expect '(' after %s name.", kind);
    consume(node, LEFT_PAREN, buf);
    params = list();
    tkn = (Token*)(*node)->data;
    if (!MATCH(tkn->type, RIGHT_PAREN)) {
        do {
            if (params->count > MAX_ARGS) {
                error(*node, "Cannot have more than 8 parameters.");
            }
            temp = consume(node, IDENTIFIER, "Expect parameter name.");
            tkn = (Token*)(*temp)->data;
            list_push(params, tkn);
            tkn = (Token*)(*node)->data;
            if (!MATCH(tkn->type, RIGHT_PAREN)) {
                (*node) = (*node)->next;
            }
        } while (MATCH(tkn->type, COMMA));
    }
    consume(node, RIGHT_PAREN, "Expect ')' after parameters.");
    memset(buf, 0, LINEBUFSIZE);
    sprintf(buf, "Expect '{' before %s body.", kind);
    consume(node, LEFT_BRACE, buf);
    body = block_statements(node);
    fnStmt = alloc(sizeof(FunStmt));
    fnStmt->name = *name;
    fnStmt->body = body;
    fnStmt->args = params;
    return new_statement(STMT_FUN, fnStmt);
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
        break;
    case LOGICAL:
        ex = ((LogicalExpr*)expr->expr)->left;
        expr_destroy(ex);
        ex = ((LogicalExpr*)expr->expr)->right;
        expr_destroy(ex);
        break;
    case CALL:
        ex = ((CallExpr*)expr->expr)->callee;
        expr_destroy(ex);
        list_destroy(((CallExpr*)expr->expr)->args);
        break;
    case VARIABLE:
    default:
        break;
    }
    fr(expr);
}

void stmt_destroy(void* stmtObj)
{
    Stmt* stmt = (Stmt*)stmtObj;
    IfElseStmt* ifStmt = NULL;
    WhileStmt* whileStmt = NULL;
    FunStmt* fnStmt = NULL;

    if (stmt != NULL) {
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
        case STMT_IF_ELSE:
            ifStmt = (IfElseStmt*)stmt->realStmt;
            stmt_destroy(ifStmt->thenStmt);
            stmt_destroy(ifStmt->elseStmt);
            expr_destroy(ifStmt->condition);
            break;
        case STMT_WHILE:
            whileStmt = (WhileStmt*)stmt->realStmt;
            stmt_destroy(whileStmt->body);
            expr_destroy(whileStmt->condition);
            break;
        case STMT_FUN:
            fnStmt = (FunStmt*)stmt->realStmt;
            list_destroy(fnStmt->args);
            stmt_destroy(fnStmt->body);
            break;
        }
        fr((void*)stmt);
    }
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
        return visitor.visitVariable(expr->expr);
    case ASSIGNMENT:
        return visitor.visitAssignment(expr->expr);
    case LOGICAL:
        return visitor.visitLogical(expr->expr);
    case CALL:
        return visitor.visitCallable(expr->expr);
    }
    return NULL;
}

void* accept(StmtVisitor visitor, Stmt* stmt)
{
    switch (stmt->type) {
    case STMT_PRINT:
        return visitor.visitPrint(stmt->realStmt);
    case STMT_EXPR:
        return visitor.visitExpression(stmt->realStmt);
    case STMT_VAR_DECLARATION:
        return visitor.visitVarDeclaration(stmt->realStmt);
    case STMT_BLOCK:
        return visitor.visitBlock(stmt->realStmt);
    case STMT_IF_ELSE:
        return visitor.visitIfElse(stmt->realStmt);
    case STMT_WHILE:
        return visitor.visitWhile(stmt->realStmt);
    case STMT_FUN:
        return visitor.visitFun(stmt->realStmt);
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
