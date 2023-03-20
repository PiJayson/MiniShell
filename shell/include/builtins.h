#ifndef _BUILTINS_H_
#define _BUILTINS_H_

#define BUILTIN_ERROR 2
#define BUILTIN_MESS_ERROR "Builtin %s error.\n"

typedef struct {
	char* name;
	int (*fun)(char**); 
} builtin_pair;

extern builtin_pair builtins_table[];

#endif /* !_BUILTINS_H_ */
