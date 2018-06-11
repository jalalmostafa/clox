#include "global.h"
#include "parse.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void except(const char* format, ...)
{
    va_list list;
    va_start(list, format);
    vfprintf(stderr, format, list);
    va_end(list);
    // if (!interactive) {
    //     exit(EXIT_FAILURE);
    // }
}

void parse_error(Token* token, const char* msg, ...)
{
    char buff[LINEBUFSIZE];
    va_list list;
    va_start(list, msg);

    memset(buff, 0, sizeof(buff));

    if (token->type == ENDOFFILE) {
        fprintf(stderr, ERROR_AT_EOF, msg);
    } else {
        sprintf(buff, ERROR_AT_LINE, token->line, msg, token->lexeme);
        vfprintf(stderr, msg, list);
    }
    va_end(list);
}
