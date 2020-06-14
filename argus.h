#ifndef ___ARGUS_H___
#define ___ARGUS_H___

/*
Definições de estados de uma tarefa
*/
#define INITIAL  0
#define RUNNING  1
#define FINISHED 2
#define KILLED   3
#define TIMEEXEC 4
#define TIMEINAC 5

/*
Definições dos nomes dos ficheiros usados
*/
#define COMMAND_FILE "aux"
#define LOG "historico"
#define OUTPUT_FILE "log"
#define OUTPUT_INDEX "log.idx"

/*
Definições dos pipes usados
*/
#define SERVER_PIPE "server_pipe"
#define CLIENT_PIPE "pipe"

/*
Definições do tipo de execução do problema
*/
#define SHELL    0
#define TERMINAL 1

/*
Definições dos tipos de funcionalidades
*/
#define TEMPO_INATIVIDADE 0
#define TEMPO_EXECUCAO    1
#define EXECUTAR 		  2
#define LISTAR            3
#define TERMINAR          4
#define HISTORICO         5
#define AJUDA             6
#define OUTPUT            7

/*
Definições dos tamanho maiximos dos diferentes arrays
*/
#define COMMAND_LENGTH_MAX 100
#define COMMAND_NUMBER_MAX 20


/*
Estrutura para um comando executar
*/
struct command{
	//estado de cada comando na tarefa
	int state;
	//pid do comando
	int pid;
	//conteudo do comando
	char command[COMMAND_LENGTH_MAX];
};

typedef struct command *COMMAND;




/*
Estrutura usada para uma funcionalidade no sistema
*/
struct function{
	//numero da funcao
	int number;
	//pid do cliente que envia
	pid_t client;
	//pid da tarefa
	int pid;
	//qual a funcionalidade
	int type;
   	int tempo;
	//numero de comandos
	int commands_number;
	//estado da tarefa
	int state;
	//lista de comandos a executar
   	struct command commands[COMMAND_NUMBER_MAX];
	//numero da tarefa
   	int tarefa;
	//linha para o comando output
	int line;
};

typedef struct function *FUNCTION;

/*
Estrutura usada para indexação do output
*/
struct outputidx{
	//offset do output
	off_t offset;
	//numero da função correspondente
	int function_number;
	//tamanho do output correspondente
	int size ;
};

typedef struct outputidx *IDX;


#endif
