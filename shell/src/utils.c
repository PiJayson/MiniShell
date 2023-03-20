#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"
#include "config.h"
#include "siparse.h"
#include "exec.h"
#include "mshell.h"

void 
printcommand(command *pcmd, int k)
{
	int flags;

	printf("\tCOMMAND %d\n",k);
	if (pcmd==NULL){
		printf("\t\t(NULL)\n");
		return;
	}

	printf("\t\targv=:");
	argseq * argseq = pcmd->args;
	do{
		printf("%s:", argseq->arg);
		argseq= argseq->next;
	}while(argseq!=pcmd->args);

	printf("\n\t\tredirections=:");
	redirseq * redirs = pcmd->redirs;
	if (redirs){
		do{	
			flags = redirs->r->flags;
			printf("(%s,%s):",redirs->r->filename,IS_RIN(flags)?"<": IS_ROUT(flags) ?">": IS_RAPPEND(flags)?">>":"??");
			redirs= redirs->next;
		} while (redirs!=pcmd->redirs);	
	}

	printf("\n");
}

void
printpipeline(pipeline * p, int k)
{
	int c;
	command ** pcmd;

	commandseq * commands= p->commands;

	printf("PIPELINE %d\n",k);
	
	if (commands==NULL){
		printf("\t(NULL)\n");
		return;
	}
	c=0;
	do{
		printcommand(commands->com,++c);
		commands= commands->next;
	}while (commands!=p->commands);

	printf("Totally %d commands in pipeline %d.\n",c,k);
	printf("Pipeline %sin background.\n", (p->flags & INBACKGROUND) ? "" : "NOT ");
}

void
printparsedline(pipelineseq * ln)
{
	int c;
	pipelineseq * ps = ln;

	if (!ln){
		printf("%s\n",SYNTAX_ERROR_STR);
		return;
	}
	c=0;

	do{
		printpipeline(ps->pipeline,++c);
		ps= ps->next;
	} while(ps!=ln);

	printf("Totally %d pipelines.",c);
	printf("\n");
}

command *
pickfirstcommand(pipelineseq * ppls)
{
	if ((ppls==NULL)
		|| (ppls->pipeline==NULL)
		|| (ppls->pipeline->commands==NULL)
		|| (ppls->pipeline->commands->com==NULL))	return NULL;
	
	return ppls->pipeline->commands->com;
}

int child_in_array(int pid){
	for(int i = 0; i < signal_data.children_pid_itr; i++) {
		if(signal_data.children_pid_arr[i] == pid) return 1;
	}
	return 0;
}

int delete_child_from_array(int pid) {
	for(int i = 0; i < signal_data.children_pid_itr; i++) {
		if(signal_data.children_pid_arr[i] == pid) {
			signal_data.children_pid_arr[i] = -2;
			signal_data.num_of_fg_children--;
			return 1;
		}
	}
	return 0;
}

int bg_add(int pid, int status) {
	if(signal_data.bg_iterator <= MAX_LINE_LENGTH / 2) {	
		signal_data.bg_arr[signal_data.bg_iterator].pid = pid;
		signal_data.bg_arr[signal_data.bg_iterator].status = status;

		signal_data.bg_iterator += 1;
		return 0;
	}
	return -1;
}

int bg_print() {
	for(int i = 0; i < signal_data.bg_iterator; i++) {
		printf(BG_PROCESS_FINISHED, signal_data.bg_arr[i].pid, signal_data.bg_arr[i].status);
	}
	signal_data.bg_iterator = 0;
	return 1;
}

void sigchild_handler(int signal) {
	int status, curr_errno;
	int pid;

	curr_errno = errno;
	
	do {
		pid = waitpid(-1, &status, WNOHANG);
		if(pid > 0) {
			if(child_in_array(pid)) {
				// Foreground process
				delete_child_from_array(pid);
			}
			else {
				// Background process
				bg_add(pid, status);
			}
		}
	} while(pid > 0);
	
	errno = curr_errno;
}

void set_signals() {
	// set empty_mask
	sigfillset(&signal_data.empty_mask);
	sigdelset(&signal_data.empty_mask, SIGCHLD);

	// block signal SIGINT, prvent from CTRL-C interruption
	// block signal SIGCHLD
	sigemptyset(&signal_data.sig_mask);
	sigaddset(&signal_data.sig_mask, SIGINT);
	sigaddset(&signal_data.sig_mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &signal_data.sig_mask, NULL);

	// add SIGCHILD handler
	signal_data.sigchild_act.sa_handler = sigchild_handler;
	signal_data.sigchild_act.sa_flags = 0;
	sigemptyset(&signal_data.sigchild_act.sa_mask);
	sigaction(SIGCHLD, &signal_data.sigchild_act, NULL);
}