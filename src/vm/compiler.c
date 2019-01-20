#include "vm/compiler.h"
#include "ds/list.h"
#include "tokenizer.h"
#include <stdio.h>

static foreach_token(List* toknz, void* toknObj)
{
    int line = -1;
    char* target = NULL;
    Token* token = (Token*)toknObj;

    if (token->line != line) {
        printf("%4d ", token->line);
        line = token->line;
    } else {
        printf("   | ");
    }
    target = token->literal == NULL ? token->lexeme : token->literal;
    printf("%2d '%.*s'\n", token->type, strlen(target), target);
}

void compile(const char* code)
{
    Tokenization toknz = toknzr(code, 0);
    list_foreach(toknz.values, foreach_token);
    toknzr_destroy(toknz);
}
