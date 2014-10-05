#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "sig_handler.h"
#include "runtime.h"


static sigset_t block_set, old_block_set;

/*
 * initialize the signal set which
 * need to be blocked
 */
void init_block_set() {
	sigemptyset(&block_set);
	sigemptyset(&old_block_set);
//	sigaddset(&block_set, SIGINT);
//	sigaddset(&block_set, SIGTSTP);
	sigaddset(&block_set, SIGCHLD); 
}

/*
 * begin to block the signal
 * set in the signal set
 */
void block_signals() {
	sigprocmask(SIG_SETMASK,&block_set,&old_block_set);
//	signal(SIGCHLD, SIG_IGN);
}

/*
 * restore to handle all the signal
 */
void unblock_signals() {
	sigprocmask(SIG_SETMASK, &old_block_set, NULL);
//	signal(SIGCHLD, chld_handler);
}

/*
 * handler to SIGINT
 */
void int_handler(int no) {
	if(current_fg_job && current_fg_job->group_id > 0) {
		kill(-current_fg_job->group_id, SIGINT);
	}
	/*
	int i;
	if(current_fg_job) {
		for(i = 0; i < current_fg_job->count; i++) {
			if(!current_fg_job->proc_seq[i].done) {
				printf("sigint -> %d\n", current_fg_job->proc_seq[i].pid);
				kill(-current_fg_job->proc_seq[i].pid, SIGINT);
			}
		}
	}
	*/
}

/*
 * handler to SIGTSTP
 */
void stp_handler(int no) {
	if(current_fg_job && current_fg_job->group_id > 0) {
		if(kill(-current_fg_job->group_id, SIGTSTP))
			perror("kill");
	}
	/*
	int i;
	if(current_fg_job) {
		for(i = 0; i < current_fg_job->count; i++) {
			if(!current_fg_job->proc_seq[i].done)
				kill(current_fg_job->proc_seq[i].pid, SIGTSTP);
		}
	}
	*/
}

/*
 * handler to SIGCHLD
 */
void chld_handler(int no) {
	int stat;
	pid_t pid;
	while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		//printf("000--%d\n", pid);
		set_done_by_pid(pid);
	}
//	CheckJobs();
//	printf("out chld\n");
//	printf("reap a process\n");
}
