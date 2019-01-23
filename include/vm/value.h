#ifndef CLOX_VALUE
#define CLOX_VALUE

typedef enum vm_object_type {
    OBJECT_STRING
} VmObjectType;

typedef unsigned char VmBoolean;
typedef double VmNumber;
typedef struct vm_object {
    VmObjectType type;
    struct vm_object* next;
} VmObject;

typedef struct vm_string {
    struct vm_object object;
    int length;
    char* chars;
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

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJECT(value) ((value).as.object)
#define AS_STRING(value) ((VmString*)AS_OBJECT(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJECT(value) ((value).type == VAL_OBJECT)

#define OBJECT_TYPE(value) (AS_OBJECT(value)->type)
#define IS_STRING(value) (is_object_type(value, OBJECT_STRING))

static int is_object_type(Value value, VmObjectType type)
{
    return IS_OBJECT(value) && (AS_OBJECT(value))->type == type;
}

typedef struct value_array {
    int count;
    int capacity;
    Value* values;
} ValueArray;

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
