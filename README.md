# **SO Shell Terminal** ![version](https://img.shields.io/badge/version-v0.5.3-yellow.svg) ![nvm version](https://img.shields.io/badge/tests-all%20passed-green.svg)
<!-- ![nvm version](https://img.shields.io/badge/tests-3%20failed-red.svg) -->

## **Introduction**
Basic shell project developed furing Operating Systems course at *TCS@JU*.

## **Last changes**
- code reorganization
- running processes in background `&`
- printing ended background processes
- prventing main process from `SIGINT` signal
- added blocking `SIGCHLD` signal when necessary

( Stage 5 is completed )

## **Changed / Created files**
- `/shell/src/mshell.c` - reading from input
- `/shell/include/exec.h` - header
- `/shell/src/exec.c` - executing program from read input
- `/shell/include/config.h` - exception macros
- `/shell/include/builtins.h` - header
- `/shell/src/builtins.c` - builtin commands
- `/shell/src/utils` - rest of comands responsible for data mangement and more

## **Features**
- running processes in background `&`
- prventing main process from `SIGINT` signal
- craeting and handling pipelines (handling `;` and `|` separations)
- redirecting I/O (reading and writing to files) of exec program
- executing bulitin commands eg. `lkill, cd, exit`
- reading data from input and properly deviding it into sections in buf
- reading long documents when size of buf is limited
- getting input by *read()*
- executing commands, by *fork()* and *execvp()* functions
- handling basic errors furing *execvp()* (eg. 'perrmision denied', 'no such file or directory')
- handling *SYNTAX_ERROR_STR*
- exiting program by *CTRL-D* or when EOF

## **Installation and Launch**
How to clone
```bash
git clone https://github.com/ITAnalyst-JU/so-shell-PiJayson.git
```

How to run
```
cd so-shell-PiJayson/shell
make
./bin/mshell
```

## **Issues**
- **NONE**

## **ToDo:**
- **NONE**