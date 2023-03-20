#ifndef _MSHELL_H_
#define _MSHELL_H_

#include <signal.h>
#include "config.h"

struct bg_pair_struct {
	int pid;
	int status;
};

struct signal_struct {
    sigset_t sig_mask;					/* SIGINT and SIGCHLD mask */
    sigset_t empty_mask;				/* empty mask */
	struct sigaction sigchild_act;		/* struct that will contain flags and handler */

	volatile int num_of_fg_children;
	int children_pid_itr;
	int children_pid_arr[MAX_LINE_LENGTH / 2];

	int bg_iterator;
	struct bg_pair_struct bg_arr[MAX_LINE_LENGTH / 2];
} signal_data;

#endif /* !_MSHELL_H_ */