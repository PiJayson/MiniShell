#ifndef _UTILS_H_
#define _UTILS_H_

#include "siparse.h"

#define BG_PROCESS_FINISHED "Background process %d terminated. (exited with status %d)\n"

void printcommand(command *, int);
void printpipeline(pipeline *, int);
void printparsedline(pipelineseq *);

command * pickfirstcommand(pipelineseq *);

int child_in_array(int);
int delete_child_from_array(int);
int bg_add(int, int);
int bg_print();

void sigchild_handler(int);
void set_signals();

#endif /* !_UTILS_H_ */
