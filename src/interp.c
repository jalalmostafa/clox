#include "interp.h"
#include "ds/list.h"

void for_stmts(void* stmtObj)
{
    Stmt* stmt = (Stmt*)stmtObj;
    Object* e = accept(EvaluateStmtVistior, stmt);
    fr(e);
}

void interp(ParsingContext ctx)
{
    list_foreach(ctx.stmts, for_stmts);
}
