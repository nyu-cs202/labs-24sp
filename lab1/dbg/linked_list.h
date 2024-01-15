#pragma once


typedef struct node_t {
    char name[10];
    int id;
    char msg[30];
    struct node_t *next; 
} node_t, *list_t;

/*
 * Modify h to point at NULL (empty list)
 */
void list_init(list_t *h);

int list_size(const list_t *h);

int list_empty(const list_t *h);

/*
 * prepend the node into the front
 */
void list_insert(list_t *h, node_t *n);

/*
 * delete first element in the lists, return the pointer to it
 */
node_t * list_delete(list_t *h, int id);


/*
 * 
 */
void print_list(const list_t *h);

