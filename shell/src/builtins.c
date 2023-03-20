#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "builtins.h"
#include "config.h"

int com_exit(char*[]);
int com_echo(char*[]);
int com_cd(char*[]);
int com_kill(char*[]);
int com_ls(char*[]);
int com_undefined(char *[]);

builtin_pair builtins_table[] = {
	{"exit",	&com_exit},
	{"lecho",	&com_echo},
	{"cd",		&com_cd},
	{"lcd",		&com_cd},
	{"lkill",	&com_kill},
	{"lls",		&com_ls},
	{NULL,NULL}
};

int com_exit(char *argv[]) {
	exit(EXIT_SUCCESS);
	return 0;
}

int com_echo(char *argv[]) {
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int com_cd(char *argv[]) {
	int size_argv = 0; 
	while(argv[++size_argv] != NULL);
	// Only name of command
	if(size_argv == 1) return chdir(getenv("HOME"));
	else if(size_argv == 2) return chdir(argv[1]);
	return -1;
}

int com_kill(char *argv[]) {
	int size_argv = 0;
	char *endptr;

	while(argv[++size_argv] != NULL);
	if(size_argv == 1 || size_argv > 3) return -1;

	int signal_number = SIGTERM;
	if(size_argv == 3) {
		errno = 0;
		signal_number = strtol((argv[1] + 1), &endptr, 10);
		if(*endptr != '\0' || errno == ERANGE) return -1;
	}

	errno = 0;
	int pid = strtol(argv[size_argv - 1], &endptr ,10);
	if(*argv[size_argv - 1] == '-' || *endptr != '\0' || errno == ERANGE) return -1;

	return kill(pid, signal_number);
}

int com_ls(char *argv[]) {
	int size_argv = 0; 
	while(argv[++size_argv] != NULL);
	// Only name of command
	if(size_argv > 1) return -1;

	char curr_path[PATH_MAX];
	getcwd(curr_path, PATH_MAX);

	if(curr_path == NULL) return -1;
	DIR *my_dir = opendir(curr_path);

	if(my_dir == NULL) return -1;
	struct dirent *read_result;

	while((read_result = readdir(my_dir)) != NULL)
	{
		if(*(read_result->d_name) == '.') continue;
		printf("%s\n", read_result->d_name);
	}
	fflush(stdout);

	return closedir(my_dir);
}

int com_undefined(char *argv[]) {
	fprintf(stderr, COMMAND_UNDEFINED, argv[0]);
	return BUILTIN_ERROR;
}
