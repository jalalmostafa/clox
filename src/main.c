#include "eval.h"
#include "except.h"
#include "global.h"
#include "interp.h"
#include "mem.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(char* name);
void header(char* name);
char* read_line(const char* prompt);
char* read_file(char* filepath);
void run(const char* code);

int main(int argc, char* argv[])
{
#ifdef _WIN32
    char separator = '\\';
#else
    char separator = '/';
#endif
    char* name = strrchr(argv[0], separator);
    char* line = NULL;
    char* buf = NULL;
    name = name != NULL ? name + 1 : name;

    if (argc > 2) {
        usage(name);
    } else {
        header(name);
        env_init_global();
        if (argc == 2) {
            buf = read_file(argv[1]);
            if (buf == NULL) {
                fprintf(stderr, "%s\n", strerror(errno));
            } else {
                run(buf);
            }
            getchar();
        } else {
            printf("Type 'exit()' to exit\n");
            for (line = read_line("> "); line != NULL && strcmp(line, "exit()\n") != 0; line = read_line("> ")) {
                run(line);
                fr(line);
            }
        }
        env_destroy(&GlobalExecutionEnvironment);
    }
    return EXIT_SUCCESS;
}

void usage(char* name)
{
    header(name);
    printf("`%s <filename>` to interpret a file.\n", name);
    printf("`%s` to launch REPL interpreter.\n", name);
}

void header(char* name)
{
    if (name != NULL) {
        printf("%s %s\n", name, VERSION);
    } else {
        printf("lox %s\n", VERSION);
    }
}

char* read_file(char* filepath)
{
    int length = 0;
    FILE* fp = fopen(filepath, "r");
    char* buf = NULL;

    if (fp == NULL) {
        except("Cannot open file\n");
        return NULL;
    }

    if (!fseek(fp, SEEK_SET, SEEK_END)) {
        length = ftell(fp);
        if (!length) {
            except("Exceeded Maximum File Size\n");
            return NULL;
        }

        rewind(fp);
    }
    length++;
    buf = (char*)malloc(length);
    memset(buf, 0, length);
    length = fread(buf, 1, length, fp);
    if (!fclose(fp)) {
        return buf;
    }
    fr(buf);
    return NULL;
}

char* read_line(const char* prompt)
{
    int size = sizeof(char) * LINEBUFSIZE;
    char* line = (char*)alloc(size);
    memset((void*)line, 0, size);
    printf("%s", prompt);
    fflush(stdin);
    fgets(line, size, stdin);
    return line;
}

void run(const char* code)
{
    interp(code);
}
