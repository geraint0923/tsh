#ifndef __TLIST_H__
#define __TLIST_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct list_item {
	void *item_val;
	struct list_item *next;
};

extern struct list_item *create_list();

extern struct list_item *create_list_item();

extern void release_list_item(struct list_item *item);

/* return previous item of appended item */
extern struct list_item *list_append_item(struct list_item *head, struct list_item *item);

extern void list_remove_item(struct list_item *prev, struct list_item *item);


#endif // __LIST_H__
