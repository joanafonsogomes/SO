#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "argus.h"
#include <sys/wait.h>


#define IN             0
#define OUT            1

char** divide_command(char *command)
{
    int conta=0,i;

for(i=0;command[i]!= '\0'; i++ ) {

    if(command[i] ==' ')
        conta++;
}

    char** str= malloc((conta+1) *sizeof(char*));

    char *tok;
    tok = strtok(command, " ");

    for (i = 0; tok; i++)
    {
        str[i]=malloc(20* sizeof(char));
        strcpy(str[i], strdup(tok));
        tok = strtok(NULL, " ");
    }
    return str;
   
}



int executa(FUNCTION f) {
    int pipeAnt = STDIN_FILENO;
    int proxPipe[2];
    int n = f->commands_number;
      printf("%s\n",(f->commands)[0]->command);
    for (int i = 0; i < n; ++i) {
        if (i < n - 1)
            pipe(proxPipe);
        if (fork() == 0) {
            if (i < n - 1) {
                close(proxPipe[IN]);
                dup2(proxPipe[OUT], STDOUT_FILENO);
                close(proxPipe[OUT]);
            }
            if (i > 0) {
                dup2(pipeAnt, STDIN_FILENO);
                close(pipeAnt);
            }
            printf("chegou\n");
            printf("%s\n",(f->commands)[i]->command);
            COMMAND aux = (f->commands)[i];
             puts(aux->command);
            char** command_divided = divide_command(aux->command);
            aux->state=RUNNING;
             puts(command_divided[0]);
            execvp(command_divided[0],command_divided);
            aux->state=FINISHED;
            _exit(1);
        }
        if (i < n - 1)
            close(proxPipe[OUT]);
        if (i > 0)
            close(pipeAnt);
        pipeAnt = proxPipe[IN];
        wait(NULL);
    }

    return 0;
}



int main(int argc, char **argv)
{
    puts("Starting server...");
	puts("Creating pipe...");
    mkfifo(SERVER_PIPE,0666);

    puts("Opening pipe...");
	int in = open(SERVER_PIPE, O_RDONLY);
	if (in < 0) {perror("open() in"); return -1;}
	
	puts("Opening file...");
	int log = open(LOG, O_CREAT | O_WRONLY | O_APPEND, 0640);
	if (log < 0) {perror("open() log"); return -1;}
	//write(log, SEP, strlen(SEP));

puts("Reading...");
    while (1) {
     FUNCTION f = malloc(sizeof(struct function));
    int bytes_read;

    while ((bytes_read = read(in, f, sizeof(struct function))) > 0)
    {
        if(f->type==EXECUTAR){
            executa(f);
            }
        write(log,f, sizeof(struct function));
    }
    //falta sair do server em condições
    //break;
    }

	/*
    puts("Reading...");
	char buf[BUFSIZE];
	int n;
	while (1) {
		n = read(in, buf, );
		if (n <= 0);
		else if (strncmp(buf, "stop", 4) == 0)
			break;
		else {
			write(log, buf, n);
			write(log, "\n", 1);
		}
	}*/

	puts("Closing server...");
	close(in);
	close(log);
	unlink(SERVER_PIPE);




    /*
    int fd;
	int flag;

    flag = mkfifo("server_pipe",0666);

    
    if(flag == 0 && fork() == 0){
		fd = open("server_pipe",O_WRONLY);
		pause();
    	_exit(0);
    } else
    {
        perror("Erro ao criar o FIFO");
    }
    
*/
/*
    int fd;
    if ((fd = open(COMMAND_FILE, O_RDWR, 0640)) >= 0)
    {
        COMMAND c = malloc(sizeof(struct command));
        int bytes_read;

        while ((bytes_read = read(fd, c, sizeof(struct command))) > 0)
        {
           
        }
    }
    else
    {
        perror("Erro a abrir o ficheiro");
    }
*/
    return 0;
}