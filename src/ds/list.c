#include "ds/list.h"
#include "mem.h"

static Node* node(void* data)
{
    Node* node = (Node*)alloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

List* list()
{
    List* list = (List*)alloc(sizeof(List));
    list->head = NULL;
    list->last = NULL;
    list->count = 0;
    return list;
}

Node* list_push(List* list, void* data)
{
    Node* newNode = node(data);
    Node* last = NULL;

    if (list == NULL) {
        return NULL;
    }
    last = list_last(list);
    if (last != NULL) {
        last->next = newNode;
        newNode->prev = last;
    }

    if (list->head == NULL) {
        list->head = newNode;
    }

    list->last = newNode;
    list->count++;
    return newNode;
}

Node* list_insert(List* list, void* data, unsigned int index)
{
    Node *newNode = NULL, *n = NULL;
    if (list == NULL) {
        return NULL;
    }

    if (index == list->count - 1) {
        return list_push(list, data);
    }

    newNode = node(data);
    list->count++;
    if (index == 0) {
        newNode->next = list->head;
        list->head->prev = newNode;
        list->head = newNode;
        return newNode;
    }

    n = list_at(list, index);
    if (n == NULL) {
        fr(newNode);
        return NULL;
    }
    n->prev->next = newNode;
    newNode->prev = n->prev;
    n->prev = newNode;
    newNode->next = n;
    return newNode;
}

int list_remove(List* list, Node* node)
{
    Node* n = NULL;
    for (n = list->head; n->next != NULL && n != node; n = n->next) {
    }

    if (n == NULL) {
        return 0;
    }

    list->count--;
    if (list->count == 0) {
        list->head = NULL;
        list->last = NULL;
    } else {
        if (n->prev != NULL) {
            n->prev->next = n->next;
        } else {
            list->head = n->next;
        }

        if (n->next != NULL) {
            n->next->prev = n->prev;
        } else {
            list->last = n->prev;
        }
    }
    fr(n);
    return 1;
}

int list_remove_at(List* list, unsigned int index)
{
    Node* n = list_at(list, index);
    if (n == NULL) {
        return 0;
    }
    n->prev->next = n->next;
    n->next->prev = n->prev;
    fr(n);
    return 1;
}

void list_clear(List* list)
{
    Node *n = NULL, *prev = NULL;
    if (list == NULL) {
        return;
    }
    n = list->last;
    while (n != NULL) {
        prev = n->prev;
        fr(n);
        n = prev;
    }
    list->count = 0;
    list->head = NULL;
    list->last = NULL;
}

Node* list_last(List* list)
{
    return list->last;
}

Node* list_at(List* list, unsigned int index)
{
    unsigned int idx = 0;
    Node* n = NULL;
    if (list == NULL) {
        return NULL;
    }

    if (index == 0) {
        return list->head;
    }

    if (index == list->count - 1) {
        return list->last;
    }
    n = list->head;
    for (; n != NULL && idx != index; n = n->next) {
        idx++;
    }
    return n;
}

void list_destroy(List* list)
{
    if (list == NULL) {
        return;
    }
    list_clear(list);
    fr(list);
}

void list_foreach(List* list, Iterator iter)
{
    Node* n = NULL;
    if (list == NULL || list->head == NULL) {
        return;
    }
    for (n = list->head; n != NULL; n = n->next) {
        iter(list, n->data);
    }
}

int list_any(List* list, Predicate predicate)
{
    Node* n = NULL;
    if (list == NULL) {
        return 0;
    }

    for (n = list->head; n != NULL; n = n->next) {
        if (predicate(n) > 0) {
            return 1;
        }
    }
    return 0;
}

void* list_pop(List* list)
{
    void* data = NULL;
    Node* node = NULL;

    if (list == NULL) {
        return NULL;
    }

    node = list->last;
    if (node != NULL) {
        data = node->data;
    }
    list_remove(list, node);
    return data;
}
