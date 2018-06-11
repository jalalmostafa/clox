#include "resolve.h"
#include "ds/list.h"
#include "ds/lldict.h"
#include "except.h"
#include "mem.h"
#include "visitor.h"
#include <stdio.h>

static void scope_begin();
static void scope_end();
static int resolve_list(List* Stmt);
static int resolve_expr(Expr* expr);
static int resolve_local(Expr* expr, Token name);
static int resolve_fun(Stmt* stmt, FunctionType type);
static void define(Token name);
static void declare(Token name);

static void* visit_var_expr_resolver(Expr* expr);
static void* visit_assign_expr_resolver(Expr* expr);
static void* visit_binary_expr_resolver(Expr* expr);
static void* visit_call_expr_resolver(Expr* expr);
static void* visit_grouping_expr_resolver(Expr* expr);
static void* visit_literal_expr_resolver(Expr* expr);
static void* visit_logical_expr_resolver(Expr* expr);
static void* visit_unary_expr_resolver(Expr* expr);

ExpressionVisitor ExpressionResolver = {
    visit_binary_expr_resolver,
    visit_unary_expr_resolver,
    visit_literal_expr_resolver,
    visit_grouping_expr_resolver,
    visit_var_expr_resolver,
    visit_assign_expr_resolver,
    visit_logical_expr_resolver,
    visit_call_expr_resolver
};

static void* visit_block_stmt_resolver(Stmt* stmt);
static void* visit_var_stmt_resolver(Stmt* Stmt);
static void* visit_fun_stmt_resolver(Stmt* Stmt);
static void* visit_expr_stmt_resolver(Stmt* Stmt);
static void* visit_if_stmt_resolver(Stmt* Stmt);
static void* visit_print_stmt_resolver(Stmt* Stmt);
static void* visit_return_stmt_resolver(Stmt* Stmt);
static void* visit_while_stmt_resolver(Stmt* Stmt);

StmtVisitor StatementResolver = {
    visit_print_stmt_resolver,
    visit_var_stmt_resolver,
    visit_expr_stmt_resolver,
    visit_block_stmt_resolver,
    visit_if_stmt_resolver,
    visit_while_stmt_resolver,
    visit_fun_stmt_resolver,
    visit_return_stmt_resolver
};

static List* scopes = NULL;
static FunctionType current_function_type = FUNCTION_TYPE_NONE;

static void scope_begin()
{
    LLDictionary* scope = NULL;
    scope = lldict();
    list_push(scopes, scope);
}

static void scope_end()
{
    LLDictionary* dict = (LLDictionary*)list_pop(scopes);
    lldict_destroy(dict);
}

static void declare(Token name)
{
    LLDictionary* scope = NULL;
    Node* node = NULL;
    int* value = NULL;
    if (scopes->count == 0) {
        return;
    }
    node = (Node*)scopes->last;
    if (node != NULL) {
        scope = (LLDictionary*)node->data;
        if (lldict_contains(scope, name.lexeme)) {
            parse_error(&name, "Variable with this name already declared in this scope.");
            return;
        }
    }

    value = (int*)alloc(sizeof(int));
    *value = 0;
    lldict_add(scope, name.lexeme, (void*)value);
}

static void define(Token name)
{
    LLDictionary* scope = NULL;
    int* value = NULL;
    Node* node = NULL;
    if (scopes->count == 0) {
        return;
    }
    node = (Node*)scopes->last;
    if (node != NULL) {
        scope = (LLDictionary*)node->data;
        value = (int*)lldict_get(scope, name.lexeme);
        *value = 1;
        lldict_set(scope, name.lexeme, value);
    }
}

static int resolve_list(List* stmts)
{
    Node* n = NULL;
    int resolved = 0;
    if (stmts != NULL || stmts->count != 0) {
        for (n = stmts->head; n != NULL; n = n->next) {
            resolved = resolve((Stmt*)n->data);
            if (!resolved) {
                break;
            }
        }
    }
    return resolved;
}

int resolve(Stmt* stmt)
{
    if (scopes == NULL) {
        scopes = list();
    }
    return accept(StatementResolver, stmt) != NULL;
}

static int resolve_expr(Expr* expr)
{
    return accept_expr(ExpressionResolver, expr) != NULL;
}

static int resolve_local(Expr* expr, Token name)
{
    int i = scopes->count - 1;
    LLDictionary* scope = NULL;
    Node* node = NULL;
    for (node = scopes->last; node != NULL; node = node->next) {
        scope = (LLDictionary*)node->next;
        if (lldict_contains(scope, name.lexeme) != NULL) {
            expr->order = scopes->count - 1 - i;
            return resolve_expr(expr);
        }
        i--;
    }
    return 1;
}

static void fun_args_iterator(void* argObj)
{
    Token* tkn = (Token*)argObj;
    declare(*tkn);
    define(*tkn);
}

static int resolve_fun(Stmt* stmt, FunctionType type)
{
    int resolved = 0;
    BlockStmt* body = NULL;
    FunctionType enclosingType = current_function_type;
    FunStmt* funStmt = (FunStmt*)stmt->realStmt;
    current_function_type = type;
    scope_begin();
    list_foreach(funStmt->args, fun_args_iterator);
    body = (BlockStmt*)funStmt->body->realStmt;
    resolved = resolve_list(body->innerStmts);
    scope_end();
    current_function_type = enclosingType;
    return resolved;
}

static void* visit_var_expr_resolver(Expr* expr)
{
    VariableExpr* varExpr = (VariableExpr*)(expr->expr);
    int* initialized = NULL;
    Node* last = (Node*)scopes->last;
    if (scopes->count != 0 && last != NULL) {
        initialized = lldict_get((LLDictionary*)last->data, varExpr->variableName.lexeme);
        if (initialized != NULL && *initialized == 0) {
            parse_error(&varExpr->variableName, "Cannot Read Local variable in its own initializer: %s\n", varExpr->variableName.lexeme);
            return NULL;
        }
    }
    return !resolve_local(expr, varExpr->variableName) ? NULL : expr;
}

static void* visit_assign_expr_resolver(Expr* expr)
{
    AssignmentExpr* assignExpr = (AssignmentExpr*)(expr->expr);
    return !resolve_expr(assignExpr->rightExpr) || !resolve_local(expr, assignExpr->variableName) ? NULL : expr;
}

static void* visit_binary_expr_resolver(Expr* expr)
{
    BinaryExpr* binary = (BinaryExpr*)expr->expr;
    return !resolve_expr(binary->leftExpr) || !resolve_expr(binary->rightExpr) ? NULL : expr;
}

static void* visit_call_expr_resolver(Expr* expr)
{
    CallExpr* call = (CallExpr*)expr->expr;
    Node* n = NULL;
    int resolved = resolve_expr(call->callee);
    if (!resolved) {
        return NULL;
    }
    if (call->args != NULL && call->args->count != 0) {
        for (n = call->args->head; n != NULL; n = n->next) {
            resolved += resolve_expr((Expr*)n->data);
            if (!resolved) {
                break;
            }
        }
    }
    return !resolved ? NULL : expr;
}

static void* visit_grouping_expr_resolver(Expr* expr)
{
    GroupingExpr* grouing = (GroupingExpr*)expr->expr;
    return !resolve_expr(grouing->expr) ? NULL : expr;
}

static void* visit_literal_expr_resolver(Expr* expr)
{
    return expr;
}

static void* visit_logical_expr_resolver(Expr* expr)
{
    LogicalExpr* logical = (LogicalExpr*)expr->expr;
    return !resolve_expr(logical->left) || !resolve_expr(logical->right) ? NULL : expr;
}

static void* visit_unary_expr_resolver(Expr* expr)
{
    UnaryExpr* unary = (UnaryExpr*)expr->expr;
    return !resolve_expr(unary->expr) ? NULL : expr;
}

static void* visit_block_stmt_resolver(Stmt* stmt)
{
    int resolved = 0;
    BlockStmt* blockStmt = (BlockStmt*)(stmt->realStmt);
    scope_begin();
    resolved = resolve_list(blockStmt->innerStmts);
    scope_end();
    return !resolved ? NULL : stmt;
}

static void* visit_var_stmt_resolver(Stmt* stmt)
{
    int resolved = 0;
    VarDeclarationStmt* varDeclStmt = (VarDeclarationStmt*)(stmt->realStmt);
    declare(varDeclStmt->varName);
    if (varDeclStmt->initializer != NULL) {
        resolved = resolve_expr(varDeclStmt->initializer);
    }
    if (resolved) {
        define(varDeclStmt->varName);
    }
    return !resolved ? NULL : stmt;
}

static void* visit_fun_stmt_resolver(Stmt* stmt)
{
    int resolved = 0;
    FunStmt* funStmt = (FunStmt*)stmt->realStmt;
    declare(funStmt->name);
    define(funStmt->name);
    resolved = resolve_fun(stmt, FUNCTION_TYPE_FUNCTION);
    return !resolved ? NULL : stmt;
}

static void* visit_expr_stmt_resolver(Stmt* stmt)
{
    ExprStmt* expr = (ExprStmt*)stmt->realStmt;
    return !resolve_expr(expr->expr) ? NULL : stmt;
}

static void* visit_if_stmt_resolver(Stmt* stmt)
{
    int resolved = 0;
    IfElseStmt* ifElse = (IfElseStmt*)stmt->realStmt;
    resolved = resolve_expr(ifElse->condition) && resolve(ifElse->thenStmt);
    if (resolved && ifElse->elseStmt != NULL) {
        resolved = resolve(ifElse->elseStmt);
    }
    return !resolved ? NULL : stmt;
}

static void* visit_print_stmt_resolver(Stmt* stmt)
{
    PrintStmt* print = (PrintStmt*)stmt->realStmt;
    return !resolve_expr(print->expr) ? NULL : stmt;
}

static void* visit_return_stmt_resolver(Stmt* stmt)
{
    ReturnStmt* retrn = (ReturnStmt*)stmt->realStmt;
    if (current_function_type != FUNCTION_TYPE_FUNCTION) {
        parse_error(&retrn->keyword, "Cannot return from top-level code.");
        return NULL;
    }
    if (retrn->value != NULL) {
        return !resolve_expr(retrn->value) ? NULL : stmt;
    }
    return stmt;
}

static void* visit_while_stmt_resolver(Stmt* stmt)
{
    WhileStmt* whle = (WhileStmt*)stmt->realStmt;
    return !resolve_expr(whle->condition) || !resolve(whle->body) ? NULL : stmt;
}
