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

int executa() {
    char** comandos[4];
    char* grep[] = {"grep", "-v", "^#", "/etc/passwd", NULL};
    char* cut[] = {"cut", "-f7", "-d:", NULL};
    char* uniq[] = {"uniq", NULL};
    char* wc[] = {"wc", "-l", NULL};
    comandos[0] = grep;
    comandos[1] = cut;
    comandos[2] = uniq;
    comandos[3] = wc;

    int pipeAnt = STDIN_FILENO;
    int proxPipe[2];
    int n = 4;

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
            execvp(comandos[i][0], comandos[i]);
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


executar "p1|p2"

executar "p3|p4"

"p1|p2|p3|p4"

int main(int argc, char **argv)
{
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

    return 0;
}