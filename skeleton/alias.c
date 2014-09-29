#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "alias.h"


struct list_item *alias_list = NULL;

void init_alias_list() {
	alias_list = (struct list_item*)malloc(sizeof(struct list_item));
	if(alias_list) {
		memset(alias_list, 0, sizeof(struct list_item));
	}
}

static void parse_alias_value(char *val, struct alias_item *item) {
	int i, count, start = 0, ss, len;
	if(val[0] == '\'') {
		for(i = strlen(val)-1; i >= 0; i--) {
			if(val[i] == '\'') {
				val[i] = 0;
				break;
			}
		}
		start = 1;
	}
	len = strlen(val + start);
	ss = 0;
	count = 0;
	for(i = start; i < start + len; i++) {
		if(val[i] != ' ') {
			if(!ss)
				count++;
			ss = 1;
		} else {
			val[i] = 0;
			ss = 0;
		}
	}
	item->argc = count;
	item->expand_argv = (char**)malloc(sizeof(char*)*item->argc);
	if(item->expand_argv) {
		ss = 0;
		count = 0;
		for(i = start; i < start + len; i++) {
			if(val[i] != 0 && !ss) {
				ss = 1;
				item->expand_argv[count++] = strdup(val + i);
			//	printf("in parsing => %d === %s\n", count-1, item->expand_argv[count-1]);
			} else if(val[i] == 0) {
				ss = 0;
			}
		}
	}
}

struct alias_item *parse_alias(char *val) {
	struct alias_item *item;
	char *dump = NULL;
	int i, len, stop;
	//printf("val ==> %s\n", val);
	assert(alias_list && val);
	len = strlen(val);
	item = (struct alias_item*)malloc(sizeof(struct alias_item));
	memset(item, 0, sizeof(struct alias_item));
	if(item) {
		dump = strdup(val);
		if(dump) {
			stop = 0;
			for(i = 0; i < len; i++)
				if(val[i] == '=') {
					dump[i] = 0;
					stop = i + 1;
					break;
				}
			item->key = strdup(dump);
			item->val = strdup(dump+stop);
			parse_alias_value(dump+stop, item);

			/*
			for(i = 0; i < item->argc; i++) {
				printf("parse result: %d => %s\n", i, item->expand_argv[i]);
			}
			*/

			free(dump);
		}
		return item;
	}
	return NULL;
}


void insert_alias_item(struct alias_item *item) {
	struct list_item *li;
	assert(alias_list && item);
	li = create_list_item();
	if(li) {
		li->item_val = (void*)item;
		list_append_item(alias_list, li);
	}
}

static struct alias_item *_find;
static char *_find_key;

static int traverse_to_find(struct alias_item *item) {
	if(!strcmp(item->key, _find_key)) {
		_find = item;
		return 0;
	}
	return 1;
}

struct alias_item *find_alias(char *key) {
	_find = NULL;
	_find_key = key;
	traverse_alias_list(traverse_to_find);
	return _find;
}

void remove_alias_item(struct alias_item *item) {
	struct list_item *li, *prev;
	struct alias_item *alias;
	assert(alias_list && item);
	li = alias_list->next;
	prev = alias_list;
	while(li) {
		alias = (struct alias_item*)li->item_val;
		if(item == alias) {
			list_remove_item(prev, li);
			release_list_item(li);
			item->item = NULL;
			return;
		}
		prev = li;
		li = li->next;
	}
}

void release_alias_item(struct alias_item *item) {
	int i;
	assert(item);
	if(item->key)
		free(item->key);
	if(item->val)
		free(item->val);
	if(item->expand_argv) {
		for(i = 0; i < item->argc; i++) 
			if(item->expand_argv[i])
				free(item->expand_argv[i]);
		free(item->expand_argv);
	}
	free(item);
}

void traverse_alias_list(int (*func)(struct alias_item*)) {
	struct list_item *item;
	struct alias_item *alias;
	assert(alias_list);
	item = alias_list->next;
	while(item) {
		alias = (struct alias_item*)item->item_val;
		if(!func(alias))
			return;
		item = item->next;
	}
}

void destroy_alias_list() {
	struct list_item *item, *next;
	assert(alias_list);
	item = alias_list->next;
	while(item) {
		next = item->next;
		release_alias_item((struct alias_item*)item->item_val);
		free(item);
		item = next;
	}
	free(alias_list);
}
