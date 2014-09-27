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
	for(num = 0; num < job->count; num++) {
		printf("%s", job->proc_seq[num].cmdline);
		if(num != job->count-1)
			printf(" | ");
	}
	printf("\n");
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
	int i;
	printf("[%d] ", job->job_id);
	//TODO print the statue of this job, like running, stopped
	if(job->bg) 
		printf("  Running       ");
	else
		printf("  Stopped       ");
	for(i = 0; i < job->count; i++) {
		printf("%s ", job->proc_seq[i].cmdline);
		if(i != job->count - 1)
			printf("| ");
	}
	if(job->bg) 
		printf(" &");
	printf("\n");
	return 1;
}

static void jobs_handler(commandT *cmd) {
	traverse_bg_job_list(job_traverse);
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

