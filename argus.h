#ifndef ___ARGUS_H___
#define ___ARGUS_H___

#define RUNNING  0
#define FINISHED 1
#define COMMAND_FILE "log"

struct command{
	int state;
	char command[200];
};

typedef struct command *COMMAND;

#endif
