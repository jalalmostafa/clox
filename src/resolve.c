#include "resolve.h"
#include "ds/list.h"
#include "ds/lldict.h"
#include "except.h"
#include "mem.h"
#include "visitor.h"
#include <stdio.h>
#include <string.h>

static void scope_begin();
static void scope_end();
static int resolve_list(List* Stmt);
static int resolve_expr(Expr* expr);
static int resolve_local(Expr* expr, Token name);
static int resolve_fun(Stmt* stmt, FunctionType type);
static int define(Token name);
static int declare(Token name);

static void* visit_var_expr_resolver(Expr* expr);
static void* visit_assign_expr_resolver(Expr* expr);
static void* visit_binary_expr_resolver(Expr* expr);
static void* visit_call_expr_resolver(Expr* expr);
static void* visit_grouping_expr_resolver(Expr* expr);
static void* visit_literal_expr_resolver(Expr* expr);
static void* visit_logical_expr_resolver(Expr* expr);
static void* visit_unary_expr_resolver(Expr* expr);
static void* visit_get_expr_resolver(Expr* expr);
static void* visit_set_expr_resolver(Expr* expr);
static void* visit_this_expr_resolver(Expr* expr);

ExpressionVisitor ExpressionResolver = {
    visit_binary_expr_resolver,
    visit_unary_expr_resolver,
    visit_literal_expr_resolver,
    visit_grouping_expr_resolver,
    visit_var_expr_resolver,
    visit_assign_expr_resolver,
    visit_logical_expr_resolver,
    visit_call_expr_resolver,
    visit_get_expr_resolver,
    visit_set_expr_resolver,
    visit_this_expr_resolver
};

static void* visit_block_stmt_resolver(Stmt* stmt);
static void* visit_var_stmt_resolver(Stmt* Stmt);
static void* visit_fun_stmt_resolver(Stmt* Stmt);
static void* visit_expr_stmt_resolver(Stmt* Stmt);
static void* visit_if_stmt_resolver(Stmt* Stmt);
static void* visit_print_stmt_resolver(Stmt* Stmt);
static void* visit_return_stmt_resolver(Stmt* Stmt);
static void* visit_while_stmt_resolver(Stmt* Stmt);
static void* visit_class_stmt_resolver(Stmt* Stmt);

StmtVisitor StatementResolver = {
    visit_print_stmt_resolver,
    visit_var_stmt_resolver,
    visit_expr_stmt_resolver,
    visit_block_stmt_resolver,
    visit_if_stmt_resolver,
    visit_while_stmt_resolver,
    visit_fun_stmt_resolver,
    visit_return_stmt_resolver,
    visit_class_stmt_resolver
};

static List* scopes = NULL;
static FunctionType current_function_type = FUNCTION_TYPE_NONE;
static ClassType current_class_type = CLASS_TYPE_NONE;

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

static int declare(Token name)
{
    LLDictionary* scope = NULL;
    Node* node = NULL;
    int* value = NULL;
    if (scopes->count == 0) {
        return 1;
    }

    node = (Node*)scopes->last;
    scope = (LLDictionary*)node->data;
    if (lldict_contains(scope, name.lexeme)) {
        parse_error(&name, "Variable with this name already declared in this scope.");
        return 0;
    }

    value = (int*)alloc(sizeof(int));
    *value = 0;
    lldict_add(scope, name.lexeme, (void*)value);
    return 1;
}

static int define(Token name)
{
    LLDictionary* scope = NULL;
    int* value = NULL;
    Node* node = NULL;
    if (scopes->count == 0) {
        return 1;
    }

    node = (Node*)scopes->last;
    scope = (LLDictionary*)node->data;
    value = (int*)lldict_get(scope, name.lexeme);
    *value = 1;
    lldict_set(scope, name.lexeme, value);
    return 1;
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
    int i = scopes->count;
    LLDictionary* scope = NULL;
    Node* node = NULL;
    for (node = scopes->last; i >= 0 && node != NULL; node = node->prev) {
        scope = (LLDictionary*)node->data;
        if (lldict_contains(scope, name.lexeme) != NULL) {
            expr->order = scopes->count != 1 ? scopes->count - 1 - i : scopes->count - i;
            return 1;
        }
        i--;
    }
    expr->order = -1;
    return 1;
}

static void fun_args_iterator(List* args, void* argObj)
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

static void* visit_get_expr_resolver(Expr* expr)
{
    GetExpr* get = (GetExpr*)expr->expr;
    return !resolve_expr(get->object) ? NULL : expr;
}

static void* visit_set_expr_resolver(Expr* expr)
{
    SetExpr* set = (SetExpr*)expr->expr;
    return !resolve_expr(set->object) || !resolve_expr(set->value) ? NULL : expr;
}

static void* visit_this_expr_resolver(Expr* expr)
{
    ThisExpr* this = (ThisExpr*)expr->expr;
    if (current_class_type == CLASS_TYPE_NONE) {
        parse_error(&this->keyword, "Cannot use 'this' outside of a class.");
        return NULL;
    }
    return !resolve_local(expr, this->keyword) ? NULL : expr;
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
    int resolved = 1;
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
    if (current_function_type == FUNCTION_TYPE_CTOR) {
        parse_error(&retrn->keyword, "Cannot return from top-level code.");
        return NULL;
    }
    if (retrn->value != NULL) {
        if (current_function_type == FUNCTION_TYPE_CTOR) {
            parse_error(&retrn->keyword, "Cannot return a value from an initializer.");
            return NULL;
        }
        return !resolve_expr(retrn->value) ? NULL : stmt;
    }
    return stmt;
}

static void* visit_while_stmt_resolver(Stmt* stmt)
{
    WhileStmt* whle = (WhileStmt*)stmt->realStmt;
    return !resolve_expr(whle->condition) || !resolve(whle->body) ? NULL : stmt;
}

static void class_foreach_method(List* methods, void* methodObj)
{
    Stmt* stmt = (Stmt*)methodObj;
    FunStmt* fun = (FunStmt*)stmt->realStmt;
    if (strcmp(fun->name.lexeme, "init") == 0) {
        resolve_fun(stmt, FUNCTION_TYPE_CTOR);
    } else {
        resolve_fun(stmt, FUNCTION_TYPE_METHOD);
    }
}

static void* visit_class_stmt_resolver(Stmt* stmt)
{
    LLDictionary* last = NULL;
    int* value = NULL;
    ClassStmt* class = (ClassStmt*)stmt->realStmt;
    ClassType enclosedClassType = current_class_type;
    current_class_type = CLASS_TYPE_CLASS;
    declare(class->name);
    define(class->name);
    scope_begin();
    last = (LLDictionary*)scopes->last->data;
    value = (int*)alloc(sizeof(int));
    *value = 1;
    lldict_add(last, "this", value);
    list_foreach(class->methods, class_foreach_method);
    scope_end();
    current_class_type = enclosedClassType;
    return stmt;
}
