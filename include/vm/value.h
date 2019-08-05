#ifndef CLOX_VALUE
#define CLOX_VALUE

#include "vm/common.h"
#include <stdio.h>

typedef enum vm_object_type {
    OBJECT_STRING,
    OBJECT_FUNCTION,
    OBJECT_NATIVE
} VmObjectType;

typedef unsigned char VmBoolean;
typedef double VmNumber;
typedef struct vm_object {
    VmObjectType type;
    struct vm_object* next;
} VmObject;

typedef struct vm_string {
    VmObject object;
    size_t length;
    char* chars;
    Hash hash;
} VmString;

typedef enum value_type {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJECT
} ValueType;

typedef struct value {
    ValueType type;
    union {
        VmBoolean boolean;
        VmNumber number;
        VmObject* object;
    } as;
} Value;

typedef struct value_array {
    int count;
    int capacity;
    Value* values;
} ValueArray;

typedef struct chunk {
    int capacity;
    int count;
    Byte* code;
    ValueArray constants;
    int* lines;
} Chunk;

typedef struct vm_function {
    VmObject obj;
    int arity;
    Chunk chunk;
    VmString* name;
} VmFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct vm_native {
    VmObject obj;
    NativeFn function;
} VmNative;

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJECT(value) ((value).as.object)
#define AS_STRING(value) ((VmString*)AS_OBJECT(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)
#define AS_FUNCTION(value) ((VmFunction*)AS_OBJECT(value))
#define AS_NATIVE(value) (((VmNative*)AS_OBJECT(value))->function)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJECT(value) ((value).type == VAL_OBJECT)

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) (is_object_type(value, OBJECT_STRING))
#define IS_FUNCTION(value) (is_object_type(value, OBJECT_FUNCTION))
#define IS_NATIVE(value) (is_object_type(value, OBJ_NATIVE))

static int is_object_type(Value value, VmObjectType type)
{
    return IS_OBJECT(value) && (AS_OBJECT(value))->type == type;
}

void value_array_init(ValueArray* array);
void value_array_write(ValueArray* array, Value value);
void value_array_free(ValueArray* array);
void value_print(Value value);
int values_equal(Value a, Value b);

Value bool_val(VmBoolean boolean);
Value nil_val();
Value number_val(VmNumber number);
Value object_val(VmObject* object);

void objects_free();

#endif
