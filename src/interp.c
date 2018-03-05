#include "interp.h"
#include "ds/list.h"
#include "mem.h"

void for_stmts(void* stmtObj)
{
    Stmt* stmt = (Stmt*)stmtObj;
    accept(EvaluateStmtVistior, stmt);
}

void interp(ParsingContext ctx)
{
    list_foreach(ctx.stmts, for_stmts);
}
