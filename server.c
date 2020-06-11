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

int write_log(FUNCTION f)
{
    int fd;
    if ((fd = open(LOG, O_WRONLY | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
        return -1;
    }
    lseek(fd, 0, SEEK_END);
    int w;
    if ((w = write(fd, f, sizeof(struct function))) < 0)
    {
        perror("error writing log");
        return -1;
    }
    printf("Comando escrito que contem: %s\n",(f->commands)[0].command);
    close(fd);
    return 1;
}

int re_write_function(FUNCTION new_function)
{
    int fd;
    FUNCTION f = malloc(sizeof(struct function));
    if ((fd = open(LOG, O_RDWR | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
        return -1;
    }

    int w;
    //int res = -1;
    int bytes_read;

    while ((bytes_read = read(fd, f, sizeof(struct function))) > 0)
    {
        if (f->pid == new_function->pid)
        {
            printf("comando iniciado na tarefa %d\n", f->pid);
            lseek(fd, -sizeof(struct function), SEEK_CUR);
            if ((w = write(fd, new_function, sizeof(struct function))) < 0)
            {
                perror("error writing log");
                return -1;
            };
        }
    }
    puts("algo mudou");
    close(fd);
    return 1;
}

/*
Função que atribui à variavel 'tmp_inat_MAX' o tempo máximo 
de inactividade de comunicação num pipe anónimo
*/
int temp_inat(FUNCTION f)
{
    printf("Tempo de inatividade num pipe: %d\n", f->tempo);
    tmp_inat_MAX = f->tempo;
    f->state = FINISHED;
    write_log(f);
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
    f->state = FINISHED;
    write_log(f);
    return 0;
}

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
    f->state = RUNNING;
    write_log(f);

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

            int count = words_count((f->commands)[i].command);
            char **command_divided = malloc((count + 1) * sizeof(char *));

            divide_command((f->commands)[i].command, command_divided);
            (f->commands)[i].state = RUNNING;
            (f->commands)[i].pid = getpid();
            re_write_function(f);
        

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
Listar
*/
int list(int pid_cliente)
{
    //encontrar o pipe pretendido (pipe+pid do cliente)
    char pid_string[32];
    sprintf(pid_string, "%d", pid_cliente);
    char nome_pipe[64] = "pipe";
    strcat(nome_pipe, pid_string);

    printf("nome do pipe: %s \n", nome_pipe);

    printf("Pedido de listar cliente: %d\n", pid_cliente);

    int fd;
    if ((fd = open(LOG, O_RDONLY | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
        return -1;
    }
    FUNCTION f = malloc(sizeof(struct function));

    //abrir o pipe com o cliente
    int out = open(nome_pipe, O_WRONLY);
    if (out < 0)
    {
        perror("Client offline");
        return -1;
    }

    int bytes_read;
    for (int p = 1; (bytes_read = read(fd, f, sizeof(struct function))) > 0; p++)
    {
        if (f->type == EXECUTAR && f->state == RUNNING)
        {

            write(out, "#", 1);
            char string[32] = "";
            sprintf(string, "%d", p);
            write(out, string, sizeof string);
            write(out, ": ", 2);
            //printf("#%d: ",p);
            //printf("executar \"");
            for (int i = 0; i < f->commands_number; i++)
            {
                write(out, (f->commands[i]).command, COMMAND_LENGTH_MAX);
                //printf("%s",(f->commands[i]).command);
                if (i < f->commands_number - 1)
                {
                    write(out, "|", 1);
                    //printf("|");
                }
            }
            write(out, "\n", 1);
            //printf("\n");
        }
    }

    sleep(1);
    write(out, "EOF", 3);
    close(fd);
    close(out);
    return 0;
}

/*
Função dedicad à funcionalidade terminar
*/
int term(int number)
{
    int fd;
    FUNCTION f = malloc(sizeof(struct function));
    if ((fd = open(LOG, O_RDWR | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
        return -1;
    }



    int res;
    if ((res = lseek(fd, (number - 1) * sizeof(struct function), SEEK_SET)) < 0)
    {
        perror("erro ao encontrar a tarefa");
        close(fd);
        return -1;
    }
    

    int rd;
    if ((rd = read(fd, f, sizeof(struct function))) < 0)
    {
        perror("error on read");
        close(fd);
        return -1;
    }

    int pid = f->pid;
    printf("qual é o pid deste pokemon? -> %d\n",(f->commands)[0].pid);
    printf("FUCKING COMANDO: %s\n",(f->commands)[0].command);
    printf("numero do processoa a terminar%d\n",pid);
    kill((f->commands)[0].pid, SIGTERM);
    kill(pid, SIGTERM);
    close(fd);
    free(f);

    return 1;
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
        perror("open fifo");
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
                if (fork() == 0)
                {
                    f->pid = getpid();
                    executa(f);
                    _exit(1);
                }
            }
            else if (f->type == TEMPO_EXECUCAO)
            {
                tempo_exec(f);
            }
            else if (f->type == TEMPO_INATIVIDADE)
            {
                temp_inat(f);
            }
            else if (f->type == LISTAR)
            {
                list(f->client);
            }
            else if (f->type == TERMINAR)
            {
                term(f->tarefa);
            }
        }
        free(f);
    }

    close(in);

    return 0;
}