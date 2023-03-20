#ifndef _EXEC_H_
#define _EXEC_H_

#include "siparse.h"

#define SYNTAX_ERROR_STR "Syntax error."
#define FILE_OR_DIR_ERROR "%s: no such file or directory\n"
#define PERMISSION_ERROR "%s: permission denied\n"
#define EXEC_ERROR "%s: exec error\n"

void getArray(argseq *, char **);
void handleError(char *);
int exec_command(command *, int, int, int *, int);
int exec_pipeline(pipeline *);
int exec_input(char *);

#endif /* !_EXEC_H_ */
