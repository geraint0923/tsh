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

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/

#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

typedef struct bgjob_l {
  pid_t pid;
  struct bgjob_l* next;
} bgjobL;

struct io_config default_io_config = { 0, 1 };

/* the pids of the background processes */
bgjobL *bgjobs = NULL;

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

/* preprocess to make the pipe done
 * file redirect will be done after fork
 * dup2 pipe -> close old pipe -> file open -> dup2 file
 * clean all open pipe after all cmd detach and then waitpid
 * */
static int pipeCmd(commandT **cmd, int n) {
	int i, pp[2], ret = 0;
	pp[0] = default_io_config.input_fd;
	pp[1] = default_io_config.output_fd;
	for(i = 0; i < n; i++) {
		cmd[i]->io_cfg.input_fd = pp[0];
		if(i != n-1) {
			//TODO genterate a pair of pipe
			//FIXME for a piped program, only one fd 
			//	will be closed after fork
			ret = pipe(pp);
		} else
			pp[1] = default_io_config.output_fd;
		cmd[i]->io_cfg.output_fd = pp[1];
	}
	if(ret)
		perror("pipe");
	return ret;
}

static int preProcCmd(commandT **cmd, int n) {
	//TODO if anyone is background, then set all to background
	//FIXME
	return pipeCmd(cmd, n);
}

static void cleanPipe(commandT **cmd, int n) {
	int i;
	for(i = 0; i < n; i++) {
		if(cmd[i]->io_cfg.input_fd != 0)
			close(cmd[i]->io_cfg.input_fd);
		if(cmd[i]->io_cfg.output_fd != 1)
			close(cmd[i]->io_cfg.output_fd);
	}
}

static void cleanAll(commandT **cmd, int n) {
	int i;
	cleanPipe(cmd, n);
	for(i = 0; i < n; i++)
		ReleaseCmdT(&cmd[i]);
}

static void waitForCmd() {
}

int total_task;
void RunCmd(commandT** cmd, int n)
{
  int i;
  total_task = n;
  if(preProcCmd(cmd, n)) {
	  cleanAll(cmd, n);
	  return;
  }
  /*
  if(n == 1)
    RunCmdFork(cmd[0], TRUE);
  else{
    RunCmdPipe(cmd[0], cmd[1]);
    for(i = 0; i < n; i++)
      ReleaseCmdT(&cmd[i]);
  }
  */
  for(i = 0; i < n; i++)
	  RunCmdFork(cmd[i], TRUE);
  cleanAll(cmd, n);
  waitForCmd();
}

void RunCmdFork(commandT* cmd, bool fork)
{
  if (cmd->argc<=0)
    return;
  if (IsBuiltIn(cmd->argv[0]))
  {
    RunBuiltInCmd(cmd);
  }
  else
  {
    RunExternalCmd(cmd, fork);
  }
}

/*
void RunCmdBg(commandT* cmd)
{
  // TODO
}

void RunCmdPipe(commandT* cmd1, commandT* cmd2)
{
}

void RunCmdRedirOut(commandT* cmd, char* file)
{
}

void RunCmdRedirIn(commandT* cmd, char* file)
{
}
*/


/*Try to run an external command*/
static void RunExternalCmd(commandT* cmd, bool fork)
{
  if (ResolveExternalCmd(cmd)){
    Exec(cmd, fork);
    //TODO add to running list
  }
  else {
    printf("%s: command not found\n", cmd->argv[0]);
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
	int fd;
	assert(forceFork == TRUE);
	if(forceFork) {
		if(!(pid = fork())) {
			//TODO deal with pipe and redirect
			if(cmd->io_cfg.input_fd != 0) {
				dup2(cmd->io_cfg.input_fd, 0);
				close(cmd->io_cfg.input_fd);
			}
			if(cmd->io_cfg.output_fd != 1) {
				dup2(cmd->io_cfg.output_fd, 1);
				close(cmd->io_cfg.output_fd);
			}

			//TODO deal with file redirect
			if(cmd->is_redirect_in) {
				fd = open(cmd->redirect_in, O_RDONLY);
				dup2(fd, 0);
				close(fd);
			}
			if(cmd->is_redirect_out) {
				fd = open(cmd->redirect_out, O_APPEND | O_CREAT | O_WRONLY,
						S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
				dup2(fd, 1);
				close(fd);
			}

			if(execv(cmd->name, cmd->argv)) {
				perror("execv");
				exit(-1);
			}
		} 
		//TODO deal with the pid
		//FIXME
		printf("pid => %d\n", pid);
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
			break;
		}
	}
}

void CheckJobs()
{
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
