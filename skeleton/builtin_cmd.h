#ifndef __BUILTIN_CMD_H__
#define __BUILTIN_CMD_H__

#include "runtime.h"

struct builtin_cmd {
	char *cmd_name;						// builtin command name
	void (*cmd_handler)(commandT *cmd);	// handle function to this builtin command
};


// builtin command list
extern struct builtin_cmd builtin_cmd_list[];

// the number of builtin commands
extern int builtin_cmd_nr;

#endif // __BUILTIN_CMD_H__
