#include "ds/list.h"
#include "eval.h"
#include "mem.h"
#include "resolve.h"

void for_stmts(List* stmts, void* stmtObj)
{
    Stmt* stmt = (Stmt*)stmtObj;
    int resolved = resolve(stmt);
    if (resolved) {
        eval(stmt);
    }
}

void interp(const char* code)
{
    Tokenization toknz = toknzr(code);
    ParsingContext ctx = parse(toknz);
    if (ctx.stmts != NULL) {
        list_foreach(ctx.stmts, for_stmts);
    }
    parser_destroy(&ctx);
    toknzr_destroy(toknz);
}

Object* interp_literal(const char* code)
{
    Object* obj = NULL;
    Tokenization toknz = toknzr(code);
    ParsingContext ctx = parse_literal(toknz);
    obj = eval_literal(ctx);
    if (obj->type != OBJ_ERROR) {
        obj->value = clone(obj->value, obj->valueSize);
    }
    parser_destroy(&ctx);
    toknzr_destroy(toknz);
    return obj;
}
