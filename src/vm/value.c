#include "vm/value.h"
#include "mem.h"
#include <stdio.h>

void value_array_init(ValueArray* array)
{
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

void value_array_write(ValueArray* array, Value value)
{
    int oldCapacity = array->capacity;
    if (array->capacity < array->count + 1) {
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(array->values, Value, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void value_array_free(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    value_array_init(array);
}

void value_print(Value value) {
    printf("%g", value);
}
