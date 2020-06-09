#ifndef ___ARGUS_H___
#define ___ARGUS_H___

#define INITIAL  0
#define RUNNING  1
#define FINISHED 2

#define COMMAND_FILE "aux"
#define LOG "log"

#define SERVER_PIPE "pipe"
#define BUFSIZE	 512

#define SHELL    0
#define TERMINAL 1


#define TEMPO_INATIVIDADE 0
#define TEMPO_EXECUCAO    1
#define EXECUTAR 		  2
#define LISTAR            3
#define TERMINAR          4
#define HISTORICO         5
#define AJUDA             6
#define OUTPUT            7

#define COMMAND_LENGTH_MAX 200
#define COMMAND_NUMBER_MAX 20



//Estrutura para um comando executar
struct command{
	int state;
	char command[COMMAND_LENGTH_MAX];
};

typedef struct command *COMMAND;



union Data {
   float tempo;
   COMMAND commands[COMMAND_NUMBER_MAX];
   int tarefa;
};


//Estrutura para uma funcionalidade no sistema
struct function{
	int type;
   	float tempo;
	int commands_number;
   	COMMAND commands[COMMAND_NUMBER_MAX];
   	int tarefa;
};

typedef struct function *FUNCTION;

int myprint(char *s);

#endif
