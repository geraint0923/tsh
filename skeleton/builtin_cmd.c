#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "builtin_cmd.h"
#include "runtime.h"
#include "alias.h"


static void printStr(const char *str) {
	int ret = write(STDOUT_FILENO, str, strlen(str));
	if(ret < 0)
		perror("write");
}

static void cd_handler(commandT *cmd) {	
	// TODO substitute the ~ character
	int ret = 0;
	struct passwd *pw;
	if(cmd->argc > 1) {
		ret = chdir(cmd->argv[1]);
	} else {
		pw = getpwuid(getuid());
		ret = chdir(pw->pw_dir);
	}
	if(ret)
		perror("cd");
}

static void echo_handler(commandT *cmd) {
	int ret, i;
	for(i = 1; i < cmd->argc; i++) {
		ret = write(cmd->io_cfg.output_fd, cmd->argv[i], strlen(cmd->argv[i]));
		if(i != cmd->argc - 1)
			ret = write(cmd->io_cfg.output_fd, " ", 1);
		if(ret == -1)
			perror("write");
	}
	ret = write(cmd->io_cfg.output_fd, "\n", 1);
	if(ret == -1)
		perror("write");
}

static void fg_handler(commandT *cmd) {
	int num;
	struct working_job *job;
	assert(cmd->argc > 1);
	sscanf(cmd->argv[1], "%d", &num);
	job = find_bg_job_by_id(num);
	if(!job)
		return;
	current_fg_job = job;
	tcsetpgrp(STDIN_FILENO, current_fg_job->group_id);
	job->bg = 0;
	kill(-job->group_id, SIGCONT);
}

static void bg_handler(commandT *cmd) {
	int num;
	struct working_job *job;
	assert(cmd->argc > 1);
	sscanf(cmd->argv[1], "%d", &num);
	job = find_bg_job_by_id(num);
	if(!job)
		return;
	job->bg = 1;
	kill(-job->group_id, SIGCONT);
}

static int job_traverse(struct working_job *job) {
	int i, done_count = 0;
	char buff[256];
	for(i = 0; i < job->count; i++) {
		if(job->proc_seq[i].done)
			done_count++;
	}
	sprintf(buff, "[%d] ", job->job_id);
	printStr(buff);
	//TODO print the statue of this job, like running, stopped
	if(done_count == job->count) {
		sprintf(buff, "  Done                    ");
	} else if(job->bg)  {
		sprintf(buff, "  Running                 ");
	} else {
		sprintf(buff, "  Stopped                 ");
	}
	printStr(buff);
	for(i = 0; i < job->count; i++) {
		sprintf(buff, "%s", job->proc_seq[i].cmdline);
		printStr(buff);
		if(i != job->count - 1)
			printStr(" | ");
	}
	if(job->bg && done_count < job->count) 
		printStr(" &");
	if(done_count == job->count) {
		remove_bg_job(job);
		release_working_job(job);
	}
	printStr("\n");
	return 1;
}

static void jobs_handler(commandT *cmd) {
	traverse_bg_job_list(job_traverse);
}

static int alias_traverse_func(struct alias_item *item) {
	char *buff = (char*)malloc(sizeof(char)*(strlen(item->key) + strlen(item->val) + 20));
	if(buff) {
		sprintf(buff, "alias %s=%s\n", item->key, item->val);
		printStr(buff);
		free(buff);
	}
	return 1;
}

static void alias_handler(commandT *cmd) {
	int i, start = 0;
	struct alias_item *item;
	if(cmd->argc == 1) {
		traverse_alias_list(alias_traverse_func);
		return;
	}
	for(i = 5; i < strlen(cmd->cmdline); i++) {
		if(cmd->cmdline[i] != ' ') {
			start = i;
			break;
		}
	}
	item = parse_alias(cmd->cmdline + start);
	if(item) {
		insert_alias_item(item);
	}
}

static void unalias_handler(commandT *cmd) {
	struct alias_item *item;
	if(cmd->argc > 1) {
		item = find_alias(cmd->argv[1]);
		if(item) {
			remove_alias_item(item);
			release_alias_item(item);
		}
	}
}


/*
 * maintain the builtin command list
 */
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

