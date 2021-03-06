/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __RUNTIME_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/************Private include**********************************************/
#include "runtime.h"
#include "io.h"
#include "builtin_cmd.h"
#include "sig_handler.h"
#include "list.h"
#include "alias.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

/*
   typedef struct bgjob_l {
   pid_t pid;
   struct bgjob_l* next;
   } bgjobL;
   */

struct io_config default_io_config = { 0, 1 };

struct list_item *bg_job_list = NULL;

struct working_job *current_fg_job = NULL;

/* the pids of the background processes */
//bgjobL *bgjobs = NULL;

/************Function Prototypes******************************************/
/* run command */
static void RunCmdFork(commandT*, bool);
/* runs an external program command after some checks */
static void RunExternalCmd(commandT*, bool);
/* resolves the path and checks for exutable flag */
static bool ResolveExternalCmd(commandT*);
/* forks and runs a external program */
static void Exec(commandT*, bool);
/* runs a builtin command */
static void RunBuiltInCmd(commandT*);
/* checks whether a command is a builtin command */
static bool IsBuiltIn(char*);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

static int fd_table[1024];
static int fd_count;


// print a string using write system call to ensure immediate print
static void printStr(const char *str) {
	int ret = write(STDOUT_FILENO, str, strlen(str));
	if(ret < 0)
		perror("write");
}

void init_job_list() {
	bg_job_list = create_list();
	assert(bg_job_list);
}

static pid_t current_group_id;

// get rid of the useless space at the end of a command string
void stripSpace(char *line) {
	int i, len;
	assert(line);
	len = strlen(line);
	for(i = len - 1; i >= 0; i++) {
		if(line[i] != ' ')
			break;
		line[i] = 0;
	}
}

struct working_job *create_working_job(commandT **cmd, int n) {
	int i;
	struct working_job *job = (struct working_job*)malloc(sizeof(struct working_job));
	assert(job);
	job->group_id = -1;
	job->job_id = -1;
	job->item = NULL;
	job->proc_seq = (struct working_proc*)malloc(sizeof(struct working_proc)*n);
	job->count = n;
	job->bg = 0;
	// ensure the memory is allocated
	assert(job->proc_seq);
	
	// to set working process one by one
	for(i = 0; i < n; i++) {
		stripSpace(cmd[i]->cmdline);
		job->proc_seq[i].cmdline = strdup(cmd[i]->cmdline);
		job->proc_seq[i].pid = cmd[i]->pid;
		if(cmd[i]->pid != -1 && job->group_id == -1)
			job->group_id = cmd[i]->pid;
		if(cmd[i]->is_builtin)
			job->proc_seq[i].done = 1;
		else
			job->proc_seq[i].done = 0;
		job->proc_seq[i].job = job;
	}
	return job;
}

void add_bg_job(struct working_job *job) {
	struct list_item *item, *prev;
	assert(bg_job_list);
	item = create_list_item();
	assert(item);
	item->item_val = (void*)job;
	job->item = item;
	prev = list_append_item(bg_job_list, item);
	assert(prev);
	if(prev == bg_job_list)
		job->job_id = 1;
	else
		job->job_id = ((struct working_job*)prev->item_val)->job_id + 1;
}

void remove_bg_job(struct working_job *job) {
	struct list_item *item, *next;
	assert(bg_job_list);
	//assert(job->item);
	if(!job->item)
		return;
	item = bg_job_list;
	while(item->next && item->next != job->item)
		item = item->next;
	if(item->next == job->item) {
		job->item = NULL;
		next = item->next->next;
		release_list_item(item->next);
		item->next = next;
	}
}

struct working_job *find_bg_job_by_id(int job_id) {
	struct list_item *item = bg_job_list->next;
	struct working_job *job;
	if(item != NULL) {
		while(item) {
			job = (struct working_job*)item->item_val;
			assert(job);
			if(job->job_id == job_id)
				return job;
			item = item->next;
		}
	}
	return NULL;
}

void set_done_by_pid(pid_t pid) {
	int i;
	struct list_item *item = bg_job_list->next;
	struct working_job *job;
	if(item != NULL) {
		while(item) {
			job = (struct working_job*)item->item_val;
			assert(job);
			for(i = 0; i < job->count; i++) {
				if(job->proc_seq[i].pid == pid) {
					job->proc_seq[i].done = 1;
					return;
				}
			}
			item = item->next;
		}
	}
}

void release_working_job(struct working_job *job) {
	int i;
	assert(job);
	for(i = 0; i < job->count; i++) {
		assert(job->proc_seq[i].cmdline);
		free(job->proc_seq[i].cmdline);
	}
	assert(job->proc_seq);
	free(job->proc_seq);
	free(job);
}

/* stop when func return 0 */
void traverse_bg_job_list(int (*func)(struct working_job*)) {
	struct list_item *item, *next;
	assert(bg_job_list && func);
	item = bg_job_list->next;
	while(item) {
		next = item->next;
		if(!func((struct working_job*)item->item_val))
			break;
		item = next;
	}
}

// release the background job list when quiing the tsh
void destroy_job_list() {
	struct list_item *item, *next;
	assert(bg_job_list);
	item = bg_job_list->next;
	while(item) {
		next = item->next;
		release_working_job((struct working_job*)item->item_val);
		free(item);
		item = next;
	}
	free(bg_job_list);
}

/* preprocess to make the pipe done
 * file redirect will be done after fork
 * dup2 pipe -> close old pipe -> file open -> dup2 file
 * clean all open pipe after all cmd detach and then waitpid
 * */
static int pipeCmd(commandT **cmd, int n) {
	int i, pp[2], ret = 0;
	pp[0] = default_io_config.input_fd;
	pp[1] = default_io_config.output_fd;
	fd_count = 0;
	for(i = 0; i < n; i++) {
		cmd[i]->io_cfg.input_fd = pp[0];
		//		cmd[i]->useless_fd[0] = pp[1];
		if(i != n-1) {
			//TODO genterate a pair of pipe
			//FIXME for a piped program, only one fd 
			//	will be closed after fork
			ret = pipe(pp);
			fd_table[fd_count++] = pp[0];
			fd_table[fd_count++] = pp[1];
		} else
			pp[1] = default_io_config.output_fd;
		cmd[i]->io_cfg.output_fd = pp[1];
		//		cmd[i]->useless_fd[1] = pp[0];
	}
	if(ret)
		perror("pipe");
	return ret;
}

static int preProcCmd(commandT **cmd, int n) {
	return pipeCmd(cmd, n);
}

// close all the pipe file descriptor after fork
// so that the processes reading from pipe 
// could exit normally
static void cleanPipe(commandT **cmd, int n) {
	int i;
	for(i = 0; i < n; i++) {
		if(cmd[i]->io_cfg.input_fd != 0)
			close(cmd[i]->io_cfg.input_fd);
		if(cmd[i]->io_cfg.output_fd != 1)
			close(cmd[i]->io_cfg.output_fd);
	}
}

static struct working_job *generateJob(commandT **cmd, int n) {
	int i;
	struct working_job *job;
	job = create_working_job(cmd, n);
	// set the current forceground job
	// only if it is not set
	// because it could be set by fg command
	if(!current_fg_job) {
		for(i = 0; i < n; i++)
			if(cmd[i]->bg) {
				current_fg_job = NULL;
				job->bg = 1;
			} else {
				current_fg_job = job;
			}
	}
	return job;
}

static void cleanAll(commandT **cmd, int n) {
	int i;
	cleanPipe(cmd, n);
	for(i = 0; i < n; i++)
		ReleaseCmdT(&cmd[i]);
}

static void waitForCmd() {
	int i, stat = 0, /*ret,*/ done_count = 0;
	char buff[1024];
	pid_t p;
	struct working_proc *proc;
	assert(current_fg_job);
	for(i = 0; i < current_fg_job->count; i++) {
		proc = &(current_fg_job->proc_seq[i]);
		assert(proc);
		if(proc->done) {
			done_count++;
		}
	}
	while(done_count < current_fg_job->count) {
		p = waitpid(-1, &stat, WUNTRACED);	
		if(p > 0) {
			for(i = 0; i < current_fg_job->count; i++) {
				if(current_fg_job->proc_seq[i].pid == p) {
					break;
				}
			}
			if(i < current_fg_job->count) {
				// reaped process in current working job
				if(WIFEXITED(stat) || WIFSIGNALED(stat)) {
					done_count++;
					current_fg_job->proc_seq[i].done = 1;
				} else if(WIFSTOPPED(stat)) {
					break;
				}		
			} else {
				// if the process is not in the current 
				// foreground job then set that process's
				// done flag to be true
				set_done_by_pid(p);
			}
		}
		
	}
	// if all the processes are done
	// release all the allocated memory
	if(done_count == current_fg_job->count) {
		remove_bg_job(current_fg_job);
		release_working_job(current_fg_job);
	} else {
		if(current_fg_job->job_id == -1) {
			add_bg_job(current_fg_job);
			printStr("\n");
		}
		sprintf(buff, "[%d]   Stopped                 ", current_fg_job->job_id);
		printStr(buff);
		for(i = 0; i < current_fg_job->count; i++) {
			sprintf(buff, "%s", current_fg_job->proc_seq[i].cmdline);
			printStr(buff);
			if(i != current_fg_job->count-1)
				printStr(" | ");
		}
		printStr("\n");
	}
}

/* 
 * substitute the symbol '~' as home directory
 * */
void subTelda(char **str) {
	struct passwd *pw;
	char *buff;
	int len;
	assert(str && *str);
	if((*str)[0] == '~' && ((*str)[1] == 0 || (*str)[1] == '/')) {
		pw = getpwuid(getuid());
		len = strlen(pw->pw_dir) + strlen(*str);
		buff = (char*)malloc(len);
		if(buff) {
			memset(buff, 0, len);
			strcpy(buff, pw->pw_dir);
			strcat(buff, (*str)+1);
			free(*str);
			*str = buff;
		}
	}
}

/*
 * expand the command which have matched alias pair
 */
int expandAlias(commandT **cmd, int arg_idx) {
	struct alias_item *item;
	int i, len, j, ret = 0;
	commandT *ct;
	item = find_alias((*cmd)->argv[arg_idx]);
	if(item) {
		// create a new commandT for expanding
		ct = CreateCmdT(item->argc - 1 + (*cmd)->argc);
		ret = item->argc - 1;
		len = 0;
		for(i = 0; i < (*cmd)->argc; i++) {
			if(i != arg_idx)
				len += strlen((*cmd)->argv[i]) + 1;
		}
		for(i = 0; i < item->argc; i++) {
			len += strlen(item->expand_argv[i]) + 1;
		}
		ct->cmdline = (char*)malloc(sizeof(char)*len);
		memset(ct->cmdline, 0, len);
		for(i = 0; i < (*cmd)->argc; i++) {
			if(i == arg_idx) {
				for(j = 0; j < item->argc; j++) {
					strcat(ct->cmdline, item->expand_argv[j]);
					if(!(i == (*cmd)->argc -1 && j == item->argc - 1))
						strcat(ct->cmdline, " ");
				}
			} else {
				strcat(ct->cmdline, (*cmd)->argv[i]);
				if(i != (*cmd)->argc - 1)
					strcat(ct->cmdline, " ");
			}
		}

		ct->redirect_in = (*cmd)->redirect_in;
		ct->redirect_out = (*cmd)->redirect_out;
		ct->is_redirect_in = (*cmd)->is_redirect_in;
		ct->is_redirect_out = (*cmd)->is_redirect_out;
		ct->bg = (*cmd)->bg;
		ct->argc = item->argc - 1 + (*cmd)->argc;
		ct->io_cfg = (*cmd)->io_cfg;
		
		// copy the arguments to the proper place
		for(i = 0; i < (*cmd)->argc; i++) {
			if(i == arg_idx) {
				for(j = 0; j < item->argc; j++) {
					ct->argv[arg_idx + j] = strdup(item->expand_argv[j]);
				}
			} else if(i < arg_idx) {
				ct->argv[i] = strdup((*cmd)->argv[i]);
			} else {
				ct->argv[i + item->argc - 1] = strdup((*cmd)->argv[i]);
			}
		}
		ReleaseCmdT(cmd);
		*cmd = ct;
	}
	return ret;
}

/*
 * check the commands to
 * see if there is any command
 * matching alias pair
 */
void checkAlias(commandT **cmd, int n) {
	int i, j, base;
	for(i = 0; i < n; i++) {
		if(!strcmp(cmd[i]->argv[0], "unalias"))
			continue;
		base = 0;
		for(j = 0; j + base < cmd[i]->argc; j++) {
			base += expandAlias(cmd + i, j + base);	
			subTelda(&(cmd[i]->argv[j+base]));
		}
	}
}

int total_task;
void RunCmd(commandT** cmd, int n)
{
  int i;
  struct working_job *job;
  current_group_id = -1;
  total_task = n;
  checkAlias(cmd, n);
  if(preProcCmd(cmd, n)) {
	  cleanAll(cmd, n);
	  return;
  }

  for(i = 0; i < n; i++) {
	  RunCmdFork(cmd[i], TRUE);
	  if(current_fg_job)
		  break;
  }
  job = generateJob(cmd, n);

  if(current_fg_job && job != current_fg_job)
	  release_working_job(job);
  cleanAll(cmd, n);
  if(current_fg_job/* && current_fg_job == job*/) {
	  waitForCmd();
  } else {
	  //TODO add to bg_job_list
	  add_bg_job(job);
  }
  current_fg_job = NULL;
}

void RunCmdFork(commandT* cmd, bool fork)
{
  if (cmd->argc<=0)
    return;
  if (IsBuiltIn(cmd->argv[0]))
  {
	  cmd->is_builtin = 1;
    RunBuiltInCmd(cmd);
  }
  else
  {
	  cmd->is_builtin = 0;
    RunExternalCmd(cmd, fork);
  }
}


/*Try to run an external command*/
static void RunExternalCmd(commandT* cmd, bool fork)
{
	char buff[256];
  if (ResolveExternalCmd(cmd)){
    Exec(cmd, fork);
    //TODO add to running list
  }
  else {
    sprintf(buff, "%s: command not found\n", cmd->argv[0]);
    printStr(buff);
    cmd->is_builtin = 1;
    cmd->pid = -1;
    fflush(stdout);
    //ReleaseCmdT(&cmd);
  }
}

/*Find the executable based on search list provided by environment variable PATH*/
static bool ResolveExternalCmd(commandT* cmd)
{
  char *pathlist, *c;
  char buf[1024];
  int i, j;
  struct stat fs;

  if(strchr(cmd->argv[0],'/') != NULL){
    if(stat(cmd->argv[0], &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(cmd->argv[0],X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(cmd->argv[0]);
          return TRUE;
        }
    }
    return FALSE;
  }
  pathlist = getenv("PATH");
  if(pathlist == NULL) return FALSE;
  i = 0;
  while(i<strlen(pathlist)){
    c = strchr(&(pathlist[i]),':');
    if(c != NULL){
      for(j = 0; c != &(pathlist[i]); i++, j++)
        buf[j] = pathlist[i];
      i++;
    }
    else{
      for(j = 0; i < strlen(pathlist); i++, j++)
        buf[j] = pathlist[i];
    }
    buf[j] = '\0';
    strcat(buf, "/");
    strcat(buf,cmd->argv[0]);
    if(stat(buf, &fs) >= 0){
      if(S_ISDIR(fs.st_mode) == 0)
        if(access(buf,X_OK) == 0){/*Whether it's an executable or the user has required permisson to run it*/
          cmd->name = strdup(buf); 
          return TRUE;
        }
    }
  }
  return FALSE; /*The command is not found or the user don't have enough priority to run.*/
}


static void Exec(commandT* cmd, bool forceFork)
{
	pid_t pid;
	int fd, i;
	assert(forceFork == TRUE);

	if(forceFork) {
		if(!(pid = fork())) {
			if(current_group_id == -1) {
				setpgid(0, 0);
			} else
				setpgid(0, current_group_id);

			if(!cmd->bg) {
				tcsetpgrp(STDIN_FILENO, getpgrp());
			}


			//TODO deal with pipe and redirect
			if(cmd->io_cfg.input_fd != 0) {
				dup2(cmd->io_cfg.input_fd, 0);
			}
			if(cmd->io_cfg.output_fd != 1) {
				dup2(cmd->io_cfg.output_fd, 1);
			}

			//TODO deal with file redirect
			if(cmd->is_redirect_in) {
				fd = open(cmd->redirect_in, O_RDONLY);
				dup2(fd, 0);
				close(fd);
			}
			if(cmd->is_redirect_out) {
				fd = open(cmd->redirect_out, O_TRUNC | O_CREAT | O_WRONLY,
						S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				dup2(fd, 1);
				close(fd);
			}

			for(i = 0; i < fd_count; i++)
				close(fd_table[i]);

			unblock_signals();
			if(execv(cmd->name, cmd->argv)) {
				perror("execv");
				exit(-1);
			}
		} else {
			if(current_group_id == -1) {
				current_group_id = pid;
				setpgid(pid, pid);
			} else
				setpgid(pid, current_group_id);
			tcsetpgrp(STDIN_FILENO, current_group_id);
			kill(pid, SIGCONT);
		}
		//TODO deal with the pid
		cmd->pid = pid;
	}
}

static bool IsBuiltIn(char* cmd)
{
	int i;
	for(i = 0; i < builtin_cmd_nr; i++) {
		if(cmd && builtin_cmd_list[i].cmd_name 
				&& !strcmp(builtin_cmd_list[i].cmd_name, cmd))
			return TRUE;
	}
  return FALSE;     
}


static void procBuiltinRedirect(commandT *cmd) {
	if(cmd->is_redirect_in) {
		cmd->io_cfg.input_fd = open(cmd->redirect_in, O_RDONLY);
	}
	if(cmd->is_redirect_out) {
		cmd->io_cfg.output_fd = open(cmd->redirect_out, O_APPEND | O_CREAT | O_WRONLY,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	}
}

static void RunBuiltInCmd(commandT* cmd)
{
	int i;
	for(i = 0; i < builtin_cmd_nr; i++) {
		if(builtin_cmd_list[i].cmd_name && builtin_cmd_list[i].cmd_handler 
				&& !strcmp(builtin_cmd_list[i].cmd_name, cmd->argv[0])) {
			procBuiltinRedirect(cmd);
			builtin_cmd_list[i].cmd_handler(cmd);
			cmd->pid = -1;
			break;
		}
	}
}

static int judge_done_func(struct working_job *job) {
	int i, done_count = 0;
	char buff[256];
	for(i = 0; i < job->count; i++) {
		if(job->proc_seq[i].done)
			done_count++;
	}
	if(done_count == job->count) {
		sprintf(buff, "[%d]   Done                    ", job->job_id);
		printStr(buff);
		for(i = 0; i < job->count; i++) {
			printStr(job->proc_seq[i].cmdline);
			if(i != job->count-1)
				printStr(" | ");
			printStr("\n");
		}
		remove_bg_job(job);
		release_working_job(job);
	}
	return 1;
}

void CheckJobs()
{
	traverse_bg_job_list(judge_done_func);
}


commandT* CreateCmdT(int n)
{
  int i;
  commandT * cd = (commandT*)malloc(sizeof(commandT) + sizeof(char *) * (n + 1));
  cd -> name = NULL;
  cd -> cmdline = NULL;
  cd -> is_redirect_in = cd -> is_redirect_out = 0;
  cd -> redirect_in = cd -> redirect_out = NULL;
  cd -> argc = n;
  cd->pid = -1;
  for(i = 0; i <=n; i++)
    cd -> argv[i] = NULL;
  return cd;
}

/*Release and collect the space of a commandT struct*/
void ReleaseCmdT(commandT **cmd){
  int i;
  if((*cmd)->name != NULL) free((*cmd)->name);
  if((*cmd)->cmdline != NULL) free((*cmd)->cmdline);
  if((*cmd)->redirect_in != NULL) free((*cmd)->redirect_in);
  if((*cmd)->redirect_out != NULL) free((*cmd)->redirect_out);
  for(i = 0; i < (*cmd)->argc; i++)
    if((*cmd)->argv[i] != NULL) free((*cmd)->argv[i]);
  free(*cmd);
}

char *getLogin() {
	return getlogin();
}

#define MAX_PATH_LEN	(1024)
static char cwd_buffer[MAX_PATH_LEN];
char *getCurrentWorkingDir() {
	char *ret = getcwd(cwd_buffer, MAX_PATH_LEN);
	if(!ret)
		cwd_buffer[0] = 0;
	return cwd_buffer;
}
