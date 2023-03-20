#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "exec.h"
#include "mshell.h"


// checks if given input is correct and return boolean value
int checkInput(char* buf, char **curr_buf_pointer, int input_length, int *read_result) {
	// is current command just an \n
	if(input_length == 1 && **curr_buf_pointer == '\n') return 0;
	else if(input_length > MAX_LINE_LENGTH || (input_length == MAX_LINE_LENGTH && buf[MAX_LINE_LENGTH-1] != '\0')){
		// if out of line then print error and read input until \n
		fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);

		while(1){
			char *end_line_pos = strchr(buf, '\n');
			if(end_line_pos == NULL){
				// cannot find \n to the end of buf
				// read again from beginning
				*read_result = read(0, buf, MAX_LINE_LENGTH);
				if(*read_result == 0) exit(EXIT_SUCCESS);
			}
			else{
				// if \n then set it to \0
				*end_line_pos = '\0';
				*curr_buf_pointer = end_line_pos + 1;
				return 0;
			}
		}
	}
	return 1;
}

int main(int argc, char *argv[])
{
	char buf[MAX_LINE_LENGTH + 1];		/* Array of read characters from input (stdin) */
	int command_length = 0;				/* Length of currently analized command */
	int read_result = 0;				/* size of buf - read input and MOVED command */
	char *curr_buf_poitner = buf;		/* Pointer to FIRST character of new command */
	int isatty_result = 0;				/* variable that informs is programm run in terminal */
	signal_data.bg_iterator = 0;

	// Set last cell to \0 to prevent from errors while using strchr()
	buf[MAX_LINE_LENGTH] = '\0';

	// Get if input is Terminal or file and handle an error if something goes wrong
	isatty_result = isatty(0);
	if(isatty_result != 1 && errno != ENOTTY) exit(EXIT_FAILURE);

	// set all necessary signals
	set_signals();

	// Reading from input and creating new processes with typed program
	while (1){

		// Check if terminal O(1)
		if(isatty_result == 1) {
			// Print all finished background children
			bg_print();

			// Print parser and set buf position to beginning to prevent from overusing syscalls
			printf(PROMPT_STR);
			fflush(stdout);
			curr_buf_poitner = buf;
		}

		// unblock for SIGCHLD signal
		sigdelset(&signal_data.sig_mask, SIGCHLD);
		sigprocmask(SIG_SETMASK, &signal_data.sig_mask, NULL);

		// If error and errno is EINTR than read again
		do {
			// Read from input
			read_result = read(0, buf + command_length, MAX_LINE_LENGTH - command_length);

		} while(read_result == -1 && errno == EINTR);

		// block for SIGCHLD signal
		sigaddset(&signal_data.sig_mask, SIGCHLD);
		sigprocmask(SIG_BLOCK, &signal_data.sig_mask, NULL);

		// reset children_pid_arr
		signal_data.children_pid_itr = 0;
		signal_data.num_of_fg_children = 0;

		// Exit if EOF
		if(read_result == 0) {
			// If there is program to be executed at the end of file then handle it
			if(command_length > 0) {
				buf[command_length] = '\0';
				exec_input(buf);
			}
			// If terminal then print new line and then end program
			if (isatty_result == 1) printf("\n");
			exit(EXIT_SUCCESS);
		}

		// set "buf size" according to read input and prevoiusly MOVED part of command
		// if there was no MOVE, the read_result is not going to be changed
		// read_result becomes a valid number of characters that can be checked
		// [MOVED command] + [read input]
		read_result += command_length;
		command_length = 0;

		// Set after last character to '\0'
		buf[read_result] = '\0';

		// Read while there is something in buf
		while((curr_buf_poitner - buf) <= read_result){	
			// Get pointer to first appearance of '\n'
			char *end_line_pos = strchr(curr_buf_poitner, '\n');

			// Check if '\n' found and if end_line_pos pointer still in range of reading
			// (end_line_pos - buf + 1) is size of potencial command
			// It occures when command is not read at once due to buf size limitations (MOVE needed)
			// or when last while iteration for done reading (then leave a loop)
			if(end_line_pos == NULL && (end_line_pos - buf + 1) <= read_result) {
				// Get length of last command prefix
				command_length = read_result - (curr_buf_poitner - buf);

				// If current command length is 0 then break and try reading more
				if(command_length == 0) {
					curr_buf_poitner = buf;
					break;
				}

				// If current command is too long than drop it
				if(!checkInput(buf, &curr_buf_poitner, command_length, &read_result)) {
					command_length = 0;
					continue;
				}
				
				// Copy command prefix to beginning and try to read again
				memmove(buf, curr_buf_poitner, (read_result - (curr_buf_poitner - buf)) * sizeof(char));
				curr_buf_poitner = buf;
				buf[command_length] = '\0';
				break;
			}
			else{
				// On success replace \n to \0 and set new length of read input
				*end_line_pos = '\0';
				command_length += end_line_pos - curr_buf_poitner + 1;
			}

			// if '\n' or input too long then continiue
			if(!checkInput(buf, &curr_buf_poitner, command_length, &read_result)) {
				command_length = 0;
				continue;
			}

			// try to exec read program with arguments
			exec_input(curr_buf_poitner);
			
			// set curr_buf_pointer to \0
			curr_buf_poitner += command_length;
			command_length = 0;
		}
	}

	return 0;
}
