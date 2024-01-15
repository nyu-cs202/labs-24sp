#include <stdio.h>
#include <assert.h>

#include "part3.h"

// Don't remove these two lines!
extern struct list_node *alloc_node(void);
extern void free_node(struct list_node *node);

// Insert a new list node with the given value right after the
// specified node.  The next pointer of the head node should point
// to the new node, and the next pointer of the new node should
// point to the old next pointer of the head node.  As an example,
// consider the following linked list (the first field is 'value', and
// the second field is 'next'):
//
// |---------|      |---------|
// |    0    |  /-> |    2    | 
// |---------| /    |---------| 
// |       ----     |   NULL  | 
// |---------|      |---------|
// 
// If the head node pointer refers to the node with the value 0,
// and list_insert(head, 1) is called, then the linked list
// structure after the list_insert call should be as follows:
//
// |---------|      |---------|      |---------|
// |    0    |  /-> |    1    |  /-> |    2    |  
// |---------| /    |---------| /    |---------| 
// |       ----     |       ----     |  NULL   |
// |---------|      |---------|      |---------|
//
// Use alloc_node to create a new node.  Don't forget to set its
// value!
void
list_insert(struct list_node *head, int value)
{
	assert(head != NULL);

	// TODO: Your code here.
	assert(0);
}

// Return a pointer to the last node in a linked list, starting
// from the given head node.  If the list only consists of the
// head node (i.e. the head node's next pointer is null), then
// simply return the head node pointer.
//
// As an example, consider the following linked list:
//
// |---------|      |---------|      |---------|
// |    0    |  /-> |    1    |  /-> |    2    |  
// |---------| /    |---------| /    |---------| 
// |       ----     |       ----     |  NULL   |
// |---------|      |---------|      |---------|
//
// If the head node pointer refers to the node with the value 0,
// list_end(head) should return a pointer to the node with the
// value of 2.
struct list_node *
list_end(struct list_node *head)
{
	assert(head != NULL);

	// TODO: Your code here.
	assert(0);
	return NULL;
}

// Return the number of nodes in a linked list, starting from the
// given head pointer.  Since the head pointer is always non-null,
// the size of a list will be at least 1.
//
// As an example, consider the following linked list:
//
// |---------|      |---------|      |---------|
// |    0    |  /-> |    1    |  /-> |    2    |  
// |---------| /    |---------| /    |---------| 
// |       ----     |       ----     |   NULL  |
// |---------|      |---------|      |---------|
//
// If the head node pointer refers to the node with the value 0,
// list_size(head) should return 3.  If the head node pointer
// refers to the node with the value 1, list_size(head) should
// return 2.
int
list_size(struct list_node *head)
{
	assert(head != NULL);

	// TODO: Your code here.
	assert(0);
	return 0;
}

// Return a pointer to the first node in the given linked list
// (starting at head) with the specified value, and store the pointer
// to its predecessor node at *predp. If no such node exists,
// return NULL and set the predecessor to NULL as well.
//
// As an example, consider the following linked list:
//
// |---------|      |---------|      |---------|
// |    0    |  /-> |    1    |  /-> |    2    |  
// |---------| /    |---------| /    |---------| 
// |       ----     |       ----     |   NULL  |
// |---------|      |---------|      |---------|
//
// If the head pointer refers to the node with the value 0, and predp
// points to a local struct list_node pointer variable, then a call
// to list_find(head, 2, predp) should return a pointer to the node
// with the value of 2 and the returned predecessor pointer (*predp)
// should point to the node with the value of 1.
//
// If the head node contains the sought-after value, then the predecessor
// pointer should be set to NULL.
struct list_node *
list_find(struct list_node *head, int value, struct list_node **predp)
{
	assert(head != NULL);
	assert(predp != NULL);

	// TODO: Your code here.
	assert(0);
	return NULL;
}

// Remove a node from the given linked list (starting at the given head)
// with the specified value.  Return 1 if a node was removed and 0
// otherwise.  If the node removed is the head node, then set the
// pointer to the head pointer to point to the new list head (which
// should be head->next).  Use free_node on the removed node.
//
// Note that instead of a pointer to a node, the passed head pointer "headp"
// is actually a pointer to a pointer.  To get the pointer to the head node,
// you will need to dereference it.  A pointer to a pointer is passed so
// that the value of the head node pointer can be changed if the head node
// is removed.
//
// As an example, consider the following linked list:
//
// |---------|      |---------|      |---------|
// |    0    |  /-> |    1    |  /-> |    2    |  
// |---------| /    |---------| /    |---------| 
// |       ----     |       ----     |   NULL  |
// |---------|      |---------|      |---------|
//
// If the head pointer refers to the node with the value 0 and
// list_remove(head, 1) is called, then the pointer to the head pointer
// should remain unchanged and the new linked list structure should
// be as follows:
//
// |---------|      |---------|
// |    0    |  /-> |    2    |  
// |---------| /    |---------| 
// |       ----     |   NULL  |
// |---------|      |---------|
//
// If we consider the original list, and list_remove(head, 0) is called,
// then the pointer to the head pointer should be set to the node
// with the value 1 and the new linked list structure should be as follows:
//
// |---------|      |---------|
// |    1    |  /-> |    2    |  
// |---------| /    |---------| 
// |       ----     |   NULL  |
// |---------|      |---------|
//
// Hint: Use list_find to get the node to remove and its predecessor,
// then manipulate the next pointers of the nodes to restructure the
// list.
// Hint: If following the above hint, don't forget to set the new pointer
// to the head node if the head node is removed.
// Hint: An alternative to the above two hints is to use a
//   pointer-to-a-pointer to iterate over the list. Code that uses a
//   pointer-to-a-pointer as an iterator is more elegant and 
//   does not need a separate case for removing the head node.
//
// Hint: Don't forget to call free_node!
int
list_remove(struct list_node **headp, int value)
{
	assert(headp != NULL);
	assert(*headp != NULL);

	// TODO: Your code here.
	assert(0);
	return 0;
}
