#include "interp.h"

Object* interp(Expr* expr)
{
    return (Object*)accept(EvalVisitor, expr);
}
