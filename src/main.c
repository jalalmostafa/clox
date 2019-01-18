#include "eval.h"
#include "except.h"
#include "global.h"
#include "interp.h"
#include "mem.h"
#include "vm/chunk.h"
#include "vm/debug.h"
#include "vm/vm.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct argvalues {
    int treewalk;
    int repl;
    char* filename;
    int help;
    int error;
} ArgValues;

typedef struct runnable_mode {
    void (*codeRunner)(const char* code);
} RunnableMode;

void usage(char* name);
void header(char* name);
char* read_line(const char* prompt);
char* read_file(char* filepath);

void run_treewalk_chunk(const char* code);
void run_treewalk_file(const char* code);
void run_vm_chunk(const char* code);
void run_vm_file(const char* code);
void vm_chunk_test();

ArgValues argparse(int argc, const char* argv[])
{
    ArgValues values;
    int i = 0;
    memset(&values, 0, sizeof(struct argvalues));
    values.treewalk = 1;
    if (argc > 3) {
        values.error = 1;
    } else if (argc == 3) {
        values.repl = 0;
        values.filename = (char*)argv[1];
        if (strncmp(argv[2], "--tree-walk", 12) == 0) {
            values.treewalk = 1;
        } else if (strncmp(argv[2], "--vm", 5) == 0) {
            values.treewalk = 0;
        } else if (strncmp(argv[2], "--help", 7) == 0) {
            values.help = 1;
        } else {
            values.error = 1;
        }
    } else if (argc == 2) {
        values.repl = 1;
        if (strncmp(argv[1], "--tree-walk", 12) == 0) {
            values.treewalk = 1;
        } else if (strncmp(argv[1], "--vm", 5) == 0) {
            values.treewalk = 0;
        } else if (strncmp(argv[1], "--help", 7) == 0) {
            values.help = 1;
        } else {
            values.repl = 0;
            values.filename = (char*)argv[1];
        }
    }

    return values;
}

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
    RunnableMode mode;
    ArgValues values = argparse(argc, argv);
    name = name != NULL ? name + 1 : name;

    if (values.error) {
        usage(name);
        return EXIT_FAILURE;
    }

    if (values.help) {
        usage(name);
        return EXIT_SUCCESS;
    }

    header(name);
    if (values.repl) {
        mode.codeRunner = values.treewalk ? run_treewalk_chunk : run_vm_chunk;
        printf("Type 'exit()' to exit\n");
        for (line = read_line("> "); line != NULL && strcmp(line, "exit()\n") != 0; line = read_line("> ")) {
            mode.codeRunner(line);
            fr(line);
        }
    } else {
        buf = read_file(argv[1]);
        if (buf == NULL) {
            fprintf(stderr, "%s\n", strerror(errno));
        } else {
            mode.codeRunner = values.treewalk ? run_treewalk_file : run_vm_file;
            mode.codeRunner(buf);
        }
    }

    return EXIT_SUCCESS;
}

void usage(char* name)
{
    header(name);
    printf("`%s <filename>`\n", name);
    printf("\t\t`%s` to launch REPL interpreter.\n", name);
    printf("\t\t--tree-walk\t\truns clox in tree walk mode");
    printf("\t\t--vm\t\truns clox in bytecode mode (default)");
    printf("\t\t--help\t\tshows this help text");
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

void run_treewalk_chunk(const char* code)
{
    interp(code);
}

void run_treewalk_file(const char* code)
{
    env_init_global();
    run_treewalk_chunk(code);
    env_destroy(&GlobalExecutionEnvironment);
}

void run_vm_chunk(const char* code)
{
    vm_chunk_test();
}

void run_vm_file(const char* code)
{
    vm_chunk_test();
}

void vm_chunk_test()
{
    int constantIdx;
    Chunk chunk;
    vm_init();
    chunk_init(&chunk);
    constantIdx = chunk_constants_add(&chunk, 1.2);
    chunk_write(&chunk, OP_CONSTANT, 1);
    chunk_write(&chunk, constantIdx, 1);
    chunk_write(&chunk, OP_RETURN, 2);
    vm_interpret(&chunk);
    chunk_disassemble(&chunk, "Test Chunk");
    vm_free();
    chunk_free(&chunk);
}