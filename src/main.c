#include "except.h"
#include "global.h"
#include "interp.h"
#include "mem.h"
#include "parse.h"
#include "readline.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(char* name);
void header(char* name);
char* read_file(char* filepath, char* buf);
void run(const char* code);

int main(int argc, char* argv[])
{
#ifdef _WIN32
    char separator = '\\';
#else
    char separator = '/';
#endif
    char* name = strrchr(argv[0], separator) + 1;
    char* line = NULL;

    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);

    if (argc > 2) {
        usage(name);
    } else {
        header(name);
        if (argc == 2) {
            char* ret = read_file(argv[1], buf);
            if (ret == NULL) {
                except("Exceeded Maximum File Size");
            }
            run(buf);
        } else {
            for (;;) {
                line = read_line("> ");
                run(line);
                fr(line);
            }
        }
    }
    return EXIT_SUCCESS;
}

void usage(char* name)
{
    header(name);
    printf("`%s <filename>` to compile a file.\n", name);
    printf("`%s` to launch compiler.\n", name);
}

void header(char* name)
{
    if (name != NULL) {
        printf("%s %s\n", name, VERSION);
    } else {
        printf("lox %s\n", VERSION);
    }
}

char* read_file(char* filepath, char* buf)
{
    int length = 0;
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        return NULL;
    }

    if (!fseek(fp, SEEK_SET, SEEK_END)) {
        length = ftell(fp);
        if (!length) {
            return NULL;
        }

        rewind(fp);
    }

    return fread(buf, BUFSIZE, 1, fp) && !fclose(fp) ? buf : NULL;
}

void run(const char* code)
{
    double* value = NULL;
    Tokenization* toknz = toknzr(code);
    Expr* expr = parse(toknz);
    Object* obj = interp(expr);
    switch (obj->type) {
    case STRING_L:
    case BOOL_L:
    case NIL_L:
    case ERROR_L:
        printf("%s\n", (char*)obj->value);
        break;
    case NUMBER_L:
        value = (double*)obj->value;
        printf("%f\n", *value);
        break;
    }
    fr(obj->value);
    fr(obj);
    destroy_expr(expr);
    toknzr_destroy(toknz);
}