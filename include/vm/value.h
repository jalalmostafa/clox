#ifndef CLOX_VALUE
#define CLOX_VALUE

typedef double Value;

typedef struct value_array {
    int count;
    int capacity;
    Value* values;
} ValueArray;

void value_array_init(ValueArray* array);
void value_array_write(ValueArray* array, Value value);
void value_array_free(ValueArray* array);
void value_print(Value value);

#endif
