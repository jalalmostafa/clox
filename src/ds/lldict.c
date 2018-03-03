#include "ds/lldict.h"
#include "ds/list.h"
#include "mem.h"
#include <string.h>

static int keyvaluepair_delete(KeyValuePair* keyValuePair);
static void for_keyvaluepair(void* data);

LLDictionary* lldict()
{
    LLDictionary* dict = (LLDictionary*)alloc(sizeof(LLDictionary));
    dict->elements = list();
    return dict;
}

void lldict_add(LLDictionary* dict, const char* key, void* value, size_t valueSize)
{
    KeyValuePair* pair = NULL;
    if (dict != NULL) {
        pair = (KeyValuePair*)alloc(sizeof(KeyValuePair));
        pair->key = clone(key, strlen(key) + 1);
        pair->value = clone(value, valueSize);
        list_push(dict->elements, pair);
    }
}

int lldict_remove(LLDictionary* dict, const char* key)
{
    Node* node = NULL;
    KeyValuePair* pair = NULL;
    int keyLength = 0;
    if (dict == NULL) {
        return 0;
    }
    keyLength = strlen(key) + 1;
    for (node = dict->elements->head; node != NULL; node = node->next) {
        pair = (KeyValuePair*)node->data;
        if (memcmp(pair->key, key, keyLength) == 0) {
            keyvaluepair_delete(pair);
            break;
        }
    }
    return list_remove(dict->elements, node);
}

void lldict_destroy(LLDictionary* dict)
{
    if (dict != NULL) {
        list_foreach(dict->elements, keyvaluepair_delete);
        list_destroy(dict->elements);
        fr(dict);
    }
}

void* lldict_get(LLDictionary* dict, const char* key)
{
    Node* node = NULL;
    KeyValuePair* pair = NULL;
    int keyLength = 0;
    if (dict == NULL) {
        return 0;
    }
    keyLength = strlen(key) + 1;
    for (node = dict->elements->head; node != NULL; node = node->next) {
        pair = (KeyValuePair*)node->data;
        if (memcmp(pair->key, key, keyLength) == 0) {
            return pair->value;
        }
    }
    return NULL;
}

static int keyvaluepair_delete(void* keyValuePairObj)
{
    KeyValuePair* keyValuePair = (KeyValuePair*)keyValuePairObj;
    if (keyValuePair != NULL) {
        fr(keyValuePair->key);
        fr(keyValuePair->value);
        fr(keyValuePair);
    }
}
