#ifndef LIST_H
#define LIST_H

typedef struct node {
    void* data;
    struct node* next;
    struct node* prev;
} Node;

typedef struct list {
    Node* head;
    Node* last;
    int count;
} List;

typedef void (*Iterator)(void* data);

typedef int (*Predicate)(Node* n);

List* list();
Node* list_push(List* list, void* data);
Node* list_insert(List* list, void* data, int index);
int list_remove(List* list, Node* n);
int list_remove_at(List* list, unsigned int index);
void list_clear(List* list);
Node* list_last(List* list);
Node* list_at(List* list, unsigned int index);
void list_foreach(List* list, Iterator iter);
void list_destroy(List* list);
int list_any(List* list, Predicate pred);

#endif
