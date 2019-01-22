#ifndef CLOX_VALUE
#define CLOX_VALUE

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

typedef unsigned char VmBoolean;
typedef double VmNumber;

typedef enum value_type {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER
} ValueType;

typedef struct value {
    ValueType type;
    union {
        VmBoolean boolean;
        VmNumber number;
    } as;
} Value;

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

#endif
