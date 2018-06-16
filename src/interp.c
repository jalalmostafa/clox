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
    Tokenization* toknz = toknzr(code);
    ParsingContext ctx = parse(toknz);
    if (ctx.stmts != NULL) {
        list_foreach(ctx.stmts, for_stmts);
    }
    parser_destroy(&ctx);
    toknzr_destroy(toknz);
}
