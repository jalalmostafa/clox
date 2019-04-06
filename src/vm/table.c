#include "vm/table.h"
#include "mem.h"
#include "vm/value.h"
#include <string.h>

void table_init(Table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void table_free(Table* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    table_init(table);
}

static Entry* find_entry(Entry* entries, int capacity, VmString* key)
{
    Entry *tombstone = NULL, *entry = NULL;
    int index = key->hash % capacity;

    for (;;) {
        entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(Table* table, int capacity)
{
    Entry* entries = (Entry*)alloc(capacity * sizeof(Entry));
    Entry *entry = NULL, *dest = NULL;
    int i;

    for (i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = nil_val();
    }

    table->count = 0;
    for (i = 0; i < table->capacity; i++) {
        entry = &table->entries[i];

        if (entry->key == NULL) {
            continue;
        }

        dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

int table_set(Table* table, VmString* key, Value value)
{
    int isNewKey, capacity;
    Entry* entry = NULL;

    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    entry = find_entry(table->entries, table->capacity, key);
    isNewKey = entry->key == NULL;
    entry->key = key;
    entry->value = value;

    if (isNewKey) {
        table->count++;
    }

    return isNewKey;
}

void table_add_all(Table* from, Table* to)
{
    int i;
    Entry* entry = NULL;

    for (i = 0; i < from->capacity; i++) {
        entry = &from->entries[i];
        if (entry->key != NULL) {
            table_set(to, entry->key, entry->value);
        }
    }
}

int table_get(Table* table, VmString* key, Value* value)
{
    Entry* entry = NULL;

    if (table->entries == NULL) {
        return 0;
    }

    entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return 0;
    }

    *value = entry->value;
    return 1;
}

int table_delete(Table* table, VmString* key)
{
    Entry* entry = NULL;
    if (table->count == 0) {
        return 0;
    }

    entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) {
        return 0;
    }

    entry->key = NULL;
    entry->value = bool_val(1);

    return 1;
}

VmString* table_find_string(Table* table, const char* chars, int length, Hash hash)
{
    unsigned int index;
    Entry* entry = NULL;

    if (table == NULL || table->entries == NULL) {
        return NULL;
    }

    index = hash % table->capacity;

    for (;;) {
        entry = &table->entries[index];

        if (entry->key == NULL) {
            return NULL;
        }

        if (entry->key->length == length && memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) & table->capacity;
    }
}
