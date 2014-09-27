#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "sig_handler.h"
#include "runtime.h"



void int_handler(int no) {
	int i;
	if(current_fg_job) {
		for(i = 0; i < current_fg_job->count; i++) {
			if(!current_fg_job->proc_seq[i].done) {
				printf("sigint -> %d\n", current_fg_job->proc_seq[i].pid);
				kill(-current_fg_job->proc_seq[i].pid, SIGINT);
			}
		}
	}
}


void stp_handler(int no) {
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

void chld_handler(int no) {
	int stat;
	wait(&stat);
	printf("reap a process\n");
}
