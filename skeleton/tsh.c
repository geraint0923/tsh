/***************************************************************************
 *  Title: MySimpleShell 
 * -------------------------------------------------------------------------
 *    Purpose: A simple shell implementation 
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: tsh.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: tsh.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.4  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.3  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
#define __MYSS_IMPL__

/************System include***********************************************/
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/************Private include**********************************************/
#include "tsh.h"
#include "io.h"
#include "interpreter.h"
#include "runtime.h"
#include "sig_handler.h"
#include "alias.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

#define BUFSIZE 80

/************Global Variables*********************************************/

/************Function Prototypes******************************************/
/* handles SIGINT and SIGSTOP signals */	
//static void sig(int);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

int main (int argc, char *argv[])
{
  /* Initialize command buffer */
  char* cmdLine = (char*)malloc(sizeof(char*)*BUFSIZE);
  

  /* shell initialization */
  if (signal(SIGINT, int_handler) == SIG_ERR) PrintPError("SIGINT");
  if (signal(SIGTSTP, stp_handler) == SIG_ERR) PrintPError("SIGTSTP");
  if (signal(SIGTTIN, SIG_IGN) == SIG_ERR) PrintPError("SIGTSTP");
  if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) PrintPError("SIGTSTP");
  if (signal(SIGCHLD, chld_handler) == SIG_ERR) PrintPError("SIGCHLD");
  //
  //tcsetpgrp (STDIN_FILENO, getpgrp());
  
  init_block_set();

  init_job_list();
  init_alias_list();

  while (!forceExit) /* repeat forever */
  {

    /* checks the status of background jobs */
	  // if there is any process recycled by shell
	  // then print out

	  //TODO unblock the signals
	  unblock_signals();
	  chld_handler(0);

	  /* print prompt */
	  //printf("%s@localhost %s $ ", getLogin(), getCurrentWorkingDir());

    /* read command line */
	tcsetpgrp(STDIN_FILENO, getpgrp());
    getCommandLine(&cmdLine, BUFSIZE);



    if(strcmp(cmdLine, "exit") == 0)
    {
      forceExit=TRUE;
      continue;
    }

    //TODO block the signals
    block_signals();


    CheckJobs();

    /* interpret command and line
     * includes executing of commands */
    Interpret(cmdLine);

  }
  destroy_job_list();
  destroy_alias_list();

  /* shell termination */
  free(cmdLine);
  return 0;
} /* end main */
