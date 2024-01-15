#include <stdlib.h>
#include "linked_list.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>


void test_delete_one() {
    list_t h;
    list_init(&h);
    node_t n;
    n.id = 0;
    strcpy(n.name, "hello");
    strcpy(n.msg, "world");
    list_insert(&h, &n);
    node_t *f = list_delete(&h, 0);
    assert(f == &n);
}


void test_delete() {
    list_t h;
    list_init(&h);
    node_t n[3];
    int i;
    for (i = 0; i < 3; i++) {
        n[i].id = i;
        list_insert(&h, &n[i]);
    }
    list_delete(&h, 1);
    assert(list_size(&h) == 2);
}

void core_dump_test() {
	int size = 0;
	list_t h;
	list_init(&h);
	size = list_size(&h);	
	printf("list size is: %d\n", size);
}

int main () {
    test_delete();
    test_delete_one();
	core_dump_test();
	printf("Pass\n");
}
