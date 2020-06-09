#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "argus.h"
#include <sys/wait.h>
#include <signal.h>

#define IN 0
#define OUT 1

int words_count(char *command){
    int conta = 0;

    for (int i = 0; command[i] != '\0'; i++)
    {
        if (command[i] == ' ')
            conta++;
    }
    return conta;
}

int divide_command(char *command, char ** str)
{
    int i = 0;
    char *tok;
    tok = strtok(command, " ");

    for (i = 0; tok; i++)
    {
        str[i] = malloc(20 * sizeof(char));
        strcpy(str[i], strdup(tok));
        tok = strtok(NULL, " ");
    }
    return i;
}

int executa(FUNCTION f)
{
    int pipeAnt = STDIN_FILENO;
    int proxPipe[2];
    int n = f->commands_number;

    for (int i = 0; i < n; ++i)
    {
        if (i < n - 1)
            pipe(proxPipe);
        if (fork() == 0)
        {
            if (i < n - 1)
            {
                close(proxPipe[IN]);
                dup2(proxPipe[OUT], STDOUT_FILENO);
                close(proxPipe[OUT]);
            }
            if (i > 0)
            {
                dup2(pipeAnt, STDIN_FILENO);
                close(pipeAnt);
            }
            printf("chegou\n");

            printf("%s\n", (f->commands)[i].command);
            int count = words_count((f->commands)[i].command);
            char **command_divided = malloc((count+1)*sizeof(char*));

            divide_command((f->commands)[i].command,command_divided);

            (f->commands)[i].state = RUNNING;
            execvp(command_divided[0], command_divided);
            (f->commands)[i].state = FINISHED;

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

void sigint_handler(int signum){
    puts("Closing server...");
    unlink(SERVER_PIPE);
    _exit(0);
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);

    puts("Starting server...");
    puts("Creating pipe...");
    mkfifo(SERVER_PIPE, 0666);

    puts("Opening pipe...");
    int in = open(SERVER_PIPE, O_RDONLY);
    if (in < 0)
    {
        perror("open() in");
        return -1;
    }

    puts("Opening file...");
    int log = open(LOG, O_CREAT | O_WRONLY | O_APPEND, 0640);
    if (log < 0)
    {
        perror("open() log");
        return -1;
    }

    puts("Reading...");
    int bytes_read;
    while (1)
    {
        FUNCTION f = malloc(sizeof(struct function));

        bytes_read = read(in, f, sizeof(struct function));
        if(bytes_read<=0);
        else{
            if (f->type == EXECUTAR)
            {
                executa(f);
            }
            write(log, f, sizeof(struct function));
        }
        free(f);
        //falta sair do server em condições
        //break;
    }

    close(in);
    close(log);

    return 0;
}