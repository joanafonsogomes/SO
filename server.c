#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include "argus.h"

#define IN 0
#define OUT 1

/*
Variável global que guarda o tempo máximo de execução de uma tarefa.
*/
int tmp_exec_MAX = 1000;

/*
Variável global que guarda o tempo máximo de inactividade de comunicação num pipe anónimo
*/
int tmp_inat_MAX = 1000;

/*
Função que "apanha" os sigint's recebidos e fecha o servidor dando unlink ao pipe.
*/
void sigint_handler(int signum)
{
    puts("Closing server...");
    unlink(SERVER_PIPE);
    _exit(0);
}
/*
void sig_alarm_handler(int signum)
{
    alarm(1);
    ++count_seconds;
    if (count_seconds > tmp_exec_MAX)
    {
        count_seconds = 0;
        kill(getpid(), SIGKILL);
    }
    printf("%d\n", count_seconds);
}
*/

/*
Função auxiliar que conta quantos espaços tem uma string
*/
int words_count(char *command)
{
    int conta = 0;

    for (int i = 0; command[i] != '\0'; i++)
    {
        if (command[i] == ' ')
            conta++;
    }
    return conta;
}

/*
Função que divide as diferentes palavras (separadas por espaços) 
e coloca-as num array de strings passado como argumento.
*/
int divide_command(char *command, char **str)
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

/*
Função destinada a executar encadeadamente os comandos 
recebidos através da struct FUNCTION.
*/
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
            // criar a ponte entre os comandos
            //if(fork() == 0){
            //não cria no ultimo o proximo pipe
            if (i < n - 1)
            {
                close(proxPipe[IN]);
                dup2(proxPipe[OUT], STDOUT_FILENO);
                close(proxPipe[OUT]);
            }
            // não executa na primeira iteração
            if (i > 0)
            {
                alarm(tmp_inat_MAX);
                dup2(pipeAnt, STDIN_FILENO);
                close(pipeAnt);
            }
            //}
            //wait(NULL);

            printf("%s\n", (f->commands)[i].command);
            int count = words_count((f->commands)[i].command);
            char **command_divided = malloc((count + 1) * sizeof(char *));

            divide_command((f->commands)[i].command, command_divided);

            (f->commands)[i].state = RUNNING;
            
            alarm(tmp_exec_MAX);
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

/*
Função que atribui à variavel 'tmp_exec_MAX' o tempo máximo 
de execução de uma tarefa.
*/
int tempo_exec(FUNCTION f)
{
    printf("Tempo de execução de uma tarefa: %d\n", f->tempo);
    tmp_exec_MAX = f->tempo;
    return 0;
}

/*
Função que atribui à variavel 'tmp_inat_MAX' o tempo máximo 
de inactividade de comunicação num pipe anónimo
*/
int temp_inat(FUNCTION f)
{
    printf("Tempo de inatividade num pipe: %d\n", f->tempo);
    tmp_inat_MAX = f->tempo;
    return 0;
}

int main(int argc, char **argv)
{
    //signal(SIGALRM, sig_alarm_handler);
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
        if (bytes_read <= 0)
            ;
        else
        {
            if (f->type == EXECUTAR)
            {
                executa(f);
            }
            else if (f->type == TEMPO_EXECUCAO)
            {
                tempo_exec(f);
            }
            else if (f->type == TEMPO_INATIVIDADE)
            {
                temp_inat(f);
            }
            write(log, f, sizeof(struct function));
        }
        free(f);
    }

    close(in);
    close(log);

    return 0;
}