all: test_linked_list

test_linked_list: linked_list.c  linked_list.h test_linked_list.c
	gcc -o test_linked_list -g linked_list.c test_linked_list.c

clean:
	rm -f test_linked_list
