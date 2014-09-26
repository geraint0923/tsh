#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "builtin_cmd.h"
#include "runtime.h"


static void cd_handler(commandT *cmd) {	
	//printf("change dir: %s\n", cmd->cmdline);
	// TODO substitute the ~ character
	int ret;
	struct passwd *pw;
	if(cmd->argc > 1 && strcmp(cmd->argv[1], "~")) {
		ret = chdir(cmd->argv[1]);
	} else {
		pw = getpwuid(getuid());
		ret = chdir(pw->pw_dir);
	}
	if(ret)
		perror("cd");
}

static void echo_handler(commandT *cmd) {
}

static void fg_handler(commandT *cmd) {
}

static void bg_handler(commandT *cmd) {
}

static void jobs_handler(commandT *cmd) {
}

static void alias_handler(commandT *cmd) {
}

static void unalias_handler(commandT *cmd) {
}


struct builtin_cmd builtin_cmd_list[] = {
	{
		.cmd_name = "cd",
		.cmd_handler = cd_handler
	},
	{
		.cmd_name = "echo",
		.cmd_handler = echo_handler
	},
	{
		.cmd_name = "fg",
		.cmd_handler = fg_handler
	},
	{
		.cmd_name = "bg",
		.cmd_handler = bg_handler 
	},
	{
		.cmd_name = "jobs",
		.cmd_handler = jobs_handler
	},
	{
		.cmd_name = "alias",
		.cmd_handler = alias_handler
	},
	{
		.cmd_name = "unalias",
		.cmd_handler = unalias_handler
	}
};


int builtin_cmd_nr = sizeof(builtin_cmd_list) / sizeof(struct builtin_cmd);
