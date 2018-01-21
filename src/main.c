#include "except.h"
#include "global.h"
#include "includes/lox-config.h"
#include "mem.h"
#include "readline.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(char* name);
void header(char* name);
char* readFile(char* filepath, char* buf);
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
            char* ret = readFile(argv[1], buf);
            if (ret == NULL) {
                except("Exceeded Maximum File Size");
            }
            run(buf);
        } else {
            for (;;) {
                line = readLine("> ");
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

char* readFile(char* filepath, char* buf)
{
    FILE* fp = fopen(filepath, "r");
    if (fp == NULL) {
        return NULL;
    }

    if (!fseek(fp, SEEK_SET, SEEK_END)) {
        int len = ftell(fp);
        if (!len) {
            return NULL;
        }

        rewind(fp);
    }

    return fread(buf, BUFSIZE, 1, fp) && !fclose(fp) ? buf : NULL;
}

void run(const char* code)
{
#ifdef DEBUG
    puts(code);
#endif
    Tokenization toknz = toknzr(code);
    toknzr_destroy(toknz);
}