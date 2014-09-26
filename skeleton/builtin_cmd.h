#ifndef __BUILTIN_CMD_H__
#define __BUILTIN_CMD_H__

#include "runtime.h"

struct builtin_cmd {
	char *cmd_name;
	void (*cmd_handler)(commandT *cmd);
};


extern struct builtin_cmd builtin_cmd_list[];

extern int builtin_cmd_nr;

#endif // __BUILTIN_CMD_H__
