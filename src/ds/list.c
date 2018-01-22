#include "list.h"
#include "../mem.h"

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
    if (list == NULL) {
        return NULL;
    }
    Node* newNode = node(data);
    Node* last = list_last(list);
    if (last != NULL) {
        last->next = newNode;
    }

    if (list->head == NULL) {
        list->head = newNode;
    }

    list->last = newNode;
    list->count++;
    return newNode;
}

Node* list_insert(List* list, void* data, int index)
{
    if (list == NULL) {
        return NULL;
    }

    if (index == list->count - 1) {
        return list_push(list, data);
    }

    Node* newNode = node(data);
    list->count++;
    if (index == 0) {
        newNode->next = list->head;
        list->head->prev = newNode;
        list->head = newNode;
        return newNode;
    }

    Node* n = list_at(list, index);
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
    n->prev->next = n->next;
    n->next->prev = n->prev;
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
    Node* n = NULL, *prev = NULL;
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
    if (list == NULL) {
        return NULL;
    }

    if (index == 0) {
        return list->head;
    }

    if (index == list->count - 1) {
        return list->last;
    }
    int idx = 0;
    Node* n = list->head;
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
    if (list == NULL || list->head != NULL) {
        return;
    }
    for (Node* n = list->head; n != NULL; n = n->next) {
        iter(n->data);
    }
}