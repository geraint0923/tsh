/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.h,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.h,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/

#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/************System include***********************************************/

/************Private include**********************************************/

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#undef EXTERN
#ifdef __RUNTIME_IMPL__
#define EXTERN
#define VAREXTERN(x, y) x = y;
#else
#define EXTERN extern
#define VAREXTERN(x, y) extern x;
#endif


#include <unistd.h>
#include "list.h"

/*
 * this struct take care of the file descriptors
 * used to redirect and pipe
 */
struct io_config {
	int input_fd;
	int output_fd;
};

// default io config for normal process
extern struct io_config default_io_config;

// indicate the current foreground job
extern struct working_job *current_fg_job;

typedef struct command_t
{
  char* name;
  char *cmdline;
  char *redirect_in, *redirect_out;
  int is_redirect_in, is_redirect_out;
  int bg;
  int argc;
  struct io_config io_cfg;
  pid_t pid;
  int is_builtin; // command not found -> builtin
  char* argv[];
} commandT;

/*
 * indicate a working job
 * could be some commands as a line
 */
struct working_job {
	int job_id;
	pid_t group_id;
	struct working_proc *proc_seq;
	struct list_item *item;
	int count;
	int bg;
};

/*
 * represent a process in a working job
 */
struct working_proc {
	char *cmdline;
	pid_t pid;
	struct working_job *job;
	int done;	// if done to be 1
};

/* to operate the foregound and background jobs list */
extern void init_job_list();

// allocate memory for a new working job
extern struct working_job *create_working_job(commandT **cmd, int n);

// add a working job to the background job list
extern void add_bg_job(struct working_job *job);

// remove working job from the background list
extern void remove_bg_job(struct working_job *job);

// reach a working by given pid
extern struct working_job *find_bg_job_by_id(int job_id);

// set the done flag of a process by given pid
extern void set_done_by_pid(pid_t pid);

// release a working job's memory
extern void release_working_job(struct working_job *job);

extern void traverse_bg_job_list(int (*func)(struct working_job*));

extern void destroy_job_list();


/************Global Variables*********************************************/

/***********************************************************************
 *  Title: Force a program exit 
 * ---------------------------------------------------------------------
 *    Purpose: Signals that a program exit is required
 ***********************************************************************/
VAREXTERN(bool forceExit, FALSE);

/************Function Prototypes******************************************/

/***********************************************************************
 *  Title: Runs a command 
 * ---------------------------------------------------------------------
 *    Purpose: Runs a command.
 *    Input: a command structure
 *    Output: void
 ***********************************************************************/
EXTERN void RunCmd(commandT**,int);

/***********************************************************************
 *  Title: Runs a command in background
 * ---------------------------------------------------------------------
 *    Purpose: Runs a command in background.
 *    Input: a command structure
 *    Output: void
 ***********************************************************************/
EXTERN void RunCmdBg(commandT*);

/***********************************************************************
 *  Title: Runs two command with a pipe
 * ---------------------------------------------------------------------
 *    Purpose: Runs two command connected with a pipe.
 *    Input: two command structure
 *    Output: void
 ***********************************************************************/
EXTERN void RunCmdPipe(commandT*, commandT*);

/***********************************************************************
 *  Title: Runs two command with output redirection
 * ---------------------------------------------------------------------
 *    Purpose: Runs a command and redirects the output to a file.
 *    Input: a command structure structure and a file name
 *    Output: void
 ***********************************************************************/
EXTERN void RunCmdRedirOut(commandT*, char*);

/***********************************************************************
 *  Title: Runs two command with input redirection
 * ---------------------------------------------------------------------
 *    Purpose: Runs a command and redirects the input to a file.
 *    Input: a command structure structure and a file name
 *    Output: void
 ***********************************************************************/
EXTERN void RunCmdRedirIn(commandT*, char*);

/***********************************************************************
 *  Title: Stop the foreground process
 * ---------------------------------------------------------------------
 *    Purpose: Stops the current foreground process if there is any.
 *    Input: void
 *    Output: void
 ***********************************************************************/
EXTERN void StopFgProc();

/***********************************************************************
 *  Title: Create a command structure 
 * ---------------------------------------------------------------------
 *    Purpose: Creates a command structure.
 *    Input: the number of arguments
 *    Output: the command structure
 ***********************************************************************/
EXTERN commandT* CreateCmdT(int);

/***********************************************************************
 *  Title: Release a command structure 
 * ---------------------------------------------------------------------
 *    Purpose: Frees the allocated memory of a command structure.
 *    Input: the command structure
 *    Output: void
 ***********************************************************************/
EXTERN void ReleaseCmdT(commandT**);

/***********************************************************************
 *  Title: Get the current working directory 
 * ---------------------------------------------------------------------
 *    Purpose: Gets the current working directory.
 *    Input: void
 *    Output: a string containing the current working directory
 ***********************************************************************/
EXTERN char* getCurrentWorkingDir();

/***********************************************************************
 *  Title: Get user name 
 * ---------------------------------------------------------------------
 *    Purpose: Gets user name logged in on the controlling terminal.
 *    Input: void
 *    Output: a string containing the user name
 ***********************************************************************/
EXTERN char* getLogin();

/***********************************************************************
 *  Title: Check the jobs 
 * ---------------------------------------------------------------------
 *    Purpose: Checks the status of the background jobs.
 *    Input: void
 *    Output: void 
 ***********************************************************************/
EXTERN void CheckJobs();

/************External Declaration*****************************************/

/**************Definition***************************************************/

#endif /* __RUNTIME_H__ */
