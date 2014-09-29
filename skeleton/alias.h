#ifndef __ALIAS_H__
#define __ALIAS_H__

#include "list.h"

struct alias_item {
	char *key;
	char *val;
	char **expand_argv;
	int argc;
	struct list_item *item;
};

extern void init_alias_list();

extern struct alias_item *parse_alias(char *val);

extern void insert_alias_item(struct alias_item *item);

extern struct alias_item *find_alias(char *key);

extern void remove_alias_item(struct alias_item *item);

extern void release_alias_item(struct alias_item *item);

extern void traverse_alias_list(int (*func)(struct alias_item*));

extern void destroy_alias_list();

#endif // __ALIAS_H__
