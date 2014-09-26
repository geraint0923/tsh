#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "list.h"

struct list_item *create_list() {
	return create_list_item();
}

struct list_item *create_list_item() {
	struct list_item *retv = NULL;
	retv = (struct list_item*)malloc(sizeof(struct list_item));
	if(!retv)
		return retv;
	memset(retv, 0, sizeof(struct list_item));
	return retv;
}

void release_list_item(struct list_item *item) {
	if(item)
		free(item);
}

void list_append_item(struct list_item *head, struct list_item *item) {
	assert(head && item);
	item->next = head->next;
	head->next = item;
}

void list_remove_item(struct list_item *prev, struct list_item *item) {
	assert(prev && item);
	prev->next = item->next;
	item->next = NULL;
}

