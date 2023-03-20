#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"
#include "exec.h"
#include "mshell.h"

// Transform arguments from list into array argv
void getArray(argseq *args_com, char** args_array) {
	argseq* first_element = args_com;	
	argseq* curr_element = args_com->next;

	args_array[0] = first_element->arg;
	int counter = 1;
	while(first_element != curr_element) {
		args_array[counter++] = curr_element->arg;

		curr_element = curr_element->next;
	}
	args_array[counter] = NULL;
}

// Handling errors thrown by execvp()
void handleError(char* first_command) {
	if(errno == ENOENT){
		fprintf(stderr, FILE_OR_DIR_ERROR, first_command);
	}
	else if(errno == EACCES){
		fprintf(stderr, PERMISSION_ERROR, first_command);
	}
	else{
		fprintf(stderr, EXEC_ERROR, first_command);
	}
}

int find_built_commands(char *command, char *args[]) {
	int iterator = 0;
	char* command_name = builtins_table[iterator].name;
	int (*command_fun)(char**) = builtins_table[iterator].fun;
	
	while(command_name != NULL && command_fun != NULL) {
		if(strcmp(command, command_name) == 0) {
			// handle error
			if(command_fun(args) == -1) fprintf(stderr, BUILTIN_MESS_ERROR, command_name);
			return 1;
		}

		command_name = builtins_table[++iterator].name;
		command_fun = builtins_table[iterator].fun;
	}
	return 0;
}

int handleRedirections(redir *curr_redir) {
	char *filename = curr_redir->filename;
	int flags = curr_redir->flags;
	
	if(IS_RIN(flags) || IS_ROUT(flags) || IS_RAPPEND(flags)) {
		int file_flags = 0;
		int stream = -1;

		if(IS_RIN(flags)) {
			file_flags = O_RDONLY;
			stream = STDIN_FILENO;
		}
		else {
			file_flags = O_CREAT | O_WRONLY | (O_APPEND * IS_RAPPEND(flags)) | (O_TRUNC * !IS_RAPPEND(flags));
			stream = STDOUT_FILENO;
		}

		int file_descriptor = open(filename, file_flags, 0666);

		// handle error
		if(file_descriptor == -1) {
			handleError(filename);
			close(file_descriptor);
			return 1;
		}

		int new_file_descriptor = dup2(file_descriptor, stream);

		// handle error
		if(new_file_descriptor == -1) {
			handleError(filename);
			close(file_descriptor);
			close(new_file_descriptor);
			return 1;
		}

		close(file_descriptor);
	}
	return 0;
}

// Redirection Input / Output for command
int redirection(command * com) {
	if(com->redirs == NULL) return 0;
	redirseq *first_element = com->redirs;
	redirseq *curr_element = first_element;
	
	do {
		redir *curr_redir = curr_element->r;

		if(handleRedirections(curr_redir) == 1) return 1;

		curr_element = curr_element->next;
	} while(curr_element != first_element);
	return 0;
}

// Trying to exec a program with arguments for given command
// Returns pipe descriptor or -1 on error
// Errors (-1) can only occur before fork
int exec_command(command *com, int prev_write_descriptor, int isLast, int *child_pid, int in_bg){
	char *args[(MAX_LINE_LENGTH + 1)/ 2 + 1];
	char *first_command;
	int pipefd[2];

	// If parser worked then try executing program
	if(com != NULL){
		first_command = com->args->arg;

		// Getting array of args from list
		getArray(com->args, args);

		// Check if command in built commands
		if(find_built_commands(first_command, args) == 1) return -1;

		// Creating pipe
		if(isLast != 1) {
			int pipe_result = pipe(pipefd);
		}

		*child_pid = fork();
		if(*child_pid == 0){
			// Child's thread

			// Redirecting decriptors for pipe
			// If current command is not first then connect to previous pipe
			if(prev_write_descriptor != -1) {
				dup2(prev_write_descriptor, STDIN_FILENO);
				close(prev_write_descriptor);
			}

			// Close old read and write descriptors
			if(isLast != 1) {
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[1]);	
				close(pipefd[0]);
			}

			// I/O redirection
			if(redirection(com) == 1) exit(EXEC_FAILURE);

			// new session and unblock signals
			if(in_bg) setsid();
			else {
				sigdelset(&signal_data.sig_mask, SIGCHLD);
				sigdelset(&signal_data.sig_mask, SIGINT);
				sigprocmask(SIG_SETMASK, &signal_data.sig_mask, NULL);
			}

			// Execute program with arguments
			int execvp_result = execvp(first_command, args);
			
			// If cannot override process with program then handle errors
			handleError(first_command);
			exit(EXEC_FAILURE);
		}
		else{
			// Parent's thread

			// pipe descriptors organization
			if(isLast != 1) {
				// close pipe's read descriptor
				close(pipefd[1]);

			}
			// If current command is not first then close previous pipe's write descriptor
			if(prev_write_descriptor != -1) close(prev_write_descriptor);
			
			// return current pipe's write descriptor
			if(isLast == 1) return 1;
			return pipefd[0];
		}
	}
	else {
		return -1;
	}
}

int exec_pipeline(pipeline *pip) {
	commandseq *first_element = pip->commands;
	int in_bg = pip->flags & INBACKGROUND;

	commandseq *curr_element = first_element;
	int prev_write_descriptor = -1;
	int numberOfChildren = 0;

	if(first_element == NULL) {
		fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
		exit(EXIT_FAILURE);
	}

	do {
		int isLast = curr_element->next == first_element ? 1 : 0;
		command *com = curr_element->com;

		int child_pid = 0;

		prev_write_descriptor = exec_command(com, prev_write_descriptor, isLast, &child_pid, in_bg);
		if(prev_write_descriptor != -1) {
			// if process in foreground than add pid to array
			if(!in_bg) {
				signal_data.num_of_fg_children++;
				signal_data.children_pid_arr[signal_data.children_pid_itr++] = child_pid;
			}
		}
		else break;

		curr_element = curr_element->next;
	} while(curr_element != first_element);

	if(!in_bg) {
		while(signal_data.num_of_fg_children > 0) {
			sigfillset(&signal_data.empty_mask);
			sigdelset(&signal_data.empty_mask, SIGCHLD);
			sigsuspend(&signal_data.empty_mask);
		}
	}

	return 0;
}

// Iterating on all pipelines separated with ';'
int exec_input(char *buf) {
	pipelineseq *first_element = parseline(buf);
	pipelineseq *curr_element = first_element;

	// Check if paseline() ended with success
	if(first_element == NULL){
		fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
		exit(EXIT_FAILURE);
	}
	
	do {
		pipeline *pip = curr_element->pipeline;

		if(pip == NULL) {
			fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
			exit(EXIT_FAILURE);
		}

		if(exec_pipeline(pip) == 1) return 1;

		curr_element = curr_element->next;
	} while(curr_element != first_element);
	return 0;
}