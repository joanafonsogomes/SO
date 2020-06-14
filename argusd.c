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
Função que procura a tarefa com um determinado pid
*/
FUNCTION find_function(int pid)
{
    int fd;
    if ((fd = open(LOG, O_RDONLY | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
    }

    int bytes_read;

    FUNCTION f = malloc(sizeof(struct function));

    while ((bytes_read = read(fd, f, sizeof(struct function))) > 0)
    {
        if (f->pid == pid)
        {
            break;
        }
    }
    return f;
}

/*
Função redireciona o stout para o ficheiro output
*/
int redirect_output(int function_number)
{
    int fd;
    fd = open(OUTPUT_FILE, O_RDWR | O_CREAT, 0644);
    off_t old_offset = lseek(fd, 0, SEEK_END);

    int fdidx;
    fdidx = open(OUTPUT_INDEX, O_RDWR | O_CREAT, 0644);
    lseek(fdidx, 0, SEEK_END);

    IDX new_index = malloc(sizeof(struct outputidx));
    new_index->offset = old_offset;
    new_index->function_number = function_number;
    new_index->size = -1;

    // fecha o stdout e redireciona para o ouput
    close(STDOUT_FILENO);
    int d;
    if ((d = dup(fd)) < 0)
    {
        perror("output erorr");
        exit(d);
    }
    close(fd);
    write(fdidx, new_index, sizeof(struct outputidx));
    close(fdidx);

    return 1;
}

/*
Função que termina o redirecionamento do output
*/
int end_output(int function_number)
{
    int fd;
    fd = open(OUTPUT_FILE, O_RDWR | O_CREAT, 0644);
    off_t new_offset = lseek(fd, 0, SEEK_END);
    close(fd);
    int fdidx;
    fdidx = open(OUTPUT_INDEX, O_RDWR | O_CREAT, 0644);

    IDX new_index = malloc(sizeof(struct outputidx));
    int bytes_read;
    while ((bytes_read = read(fdidx, new_index, sizeof(struct outputidx))) > 0)
    {
        if (new_index->function_number == function_number)
        {
            new_index->size = new_offset - new_index->offset;
            lseek(fdidx, -(sizeof(struct outputidx)), SEEK_CUR);
            write(fdidx, new_index, sizeof(struct outputidx));
        }
    }
    close(fdidx);
    return 1;
}

/*
Função que atualiza o estado de uma tarefa
*/
int re_write_function(FUNCTION new_function)
{
    int fd;
    FUNCTION f = malloc(sizeof(struct function));
    if ((fd = open(LOG, O_RDWR | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
        return -1;
    }

    lseek(fd, (new_function->number - 1) * sizeof(struct function), SEEK_CUR);
    int w;
    //int res = -1;
    int bytes_read;

    while ((bytes_read = read(fd, f, sizeof(struct function))) > 0)
    {
        if (f->pid == new_function->pid)
        {
            lseek(fd, -sizeof(struct function), SEEK_CUR);
            if ((w = write(fd, new_function, sizeof(struct function))) < 0)
            {
                perror("error writing log");
                return -1;
            };
        }
    }
    free(f);
    close(fd);
    return 1;
}

/*
Função que "apanha" os sigint's recebidos e fecha o servidor dando unlink ao pipe.
*/
void sigALRM_handler_exec(int signum)
{
    FUNCTION f = find_function(getpid());
    int pid = f->pid;
    for (int i = 0; i < f->commands_number; i++)
    {
        if ((f->commands)[i].state == RUNNING)
        {
            kill((f->commands)[i].pid, SIGTERM);
        }
    }

    f->state = TIMEEXEC;
    re_write_function(f);
    end_output(f->number);
    kill(pid, SIGTERM);
    _exit(TIMEEXEC);
}

/*
Função que "apanha" os sigint's recebidos e fecha o servidor dando unlink ao pipe.
*/
void sigALRM_handler_inac(int signum)
{
    FUNCTION f = find_function(getpid());
    int pid = f->pid;
    for (int i = 0; i < f->commands_number; i++)
    {
        if ((f->commands)[i].state == RUNNING && (f->commands)[i].pid)
        {
            kill((f->commands)[i].pid, SIGTERM);
        }
    }

    f->state = TIMEINAC;
    re_write_function(f);
    end_output(f->number);
    kill(pid, SIGTERM);
    _exit(TIMEINAC);
}

/*
Função que adiciona uma tarefa ao ficheiro de historico
*/
int write_log(FUNCTION f)
{
    int fd;
    if ((fd = open(LOG, O_WRONLY | O_CREAT, 0644)) < 0)
    {
        perror("error open log");
        return -1;
    }
    off_t offset = lseek(fd, 0, SEEK_END);
    int number = offset / (sizeof(struct function));
    number++;
    int w;
    if ((w = write(fd, f, sizeof(struct function))) < 0)
    {
        perror("error writing log");
        return -1;
    }
    printf("Tarefa nº: %d\n", number);

    //encontrar o pipe pretendido (pipe+pid do cliente)
    char pid_string[32];
    sprintf(pid_string, "%d", f->client);
    char nome_pipe[64] = "pipe";
    strcat(nome_pipe, pid_string);

    //abrir o pipe com o cliente
    int out = open(nome_pipe, O_WRONLY);
    if (out < 0)
    {
        perror("Client offline");
        return -1;
    }

    write(out, "nova tarefa #", 13);

    char string[32] = "";
    sprintf(string, "%d", number);
    write(out, string, sizeof string);
    write(out, "\n", 1);

    close(out);
    close(fd);
    return number;
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
    //write_log(f);
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
    //write_log(f);
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
        str[i] = malloc(COMMAND_LENGTH_MAX * sizeof(char *));
        str[i] = malloc(20 * sizeof(char));
        strcpy(str[i], strdup(tok));
        tok = strtok(NULL, " ");
    }
    str[i] = NULL;
    return i;
}

/*
Função destinada a executar encadeadamente os comandos 
recebidos através da struct FUNCTION.
*/
int executa(FUNCTION f)
{
    if (fork() == 0)
    {
        int status;
        int pid;
        if ((pid = fork()) == 0)
        {
            f->state = RUNNING;
            f->pid = getpid();
            pid = f->pid;
            f->number = write_log(f);

            int pipeAnt = STDIN_FILENO;
            int proxPipe[2];
            int n = f->commands_number;
            int status = 1;

            signal(SIGALRM, sigALRM_handler_exec);
            alarm(tmp_exec_MAX);

            f->pid = getpid();
            for (int i = 0; i < n; ++i)
            {
                if (i < n - 1)
                    pipe(proxPipe);
                if (fork() == 0)
                {
                    signal(SIGALRM, sigALRM_handler_inac);
                    alarm(tmp_inat_MAX);
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
                        dup2(pipeAnt, STDIN_FILENO);
                        close(pipeAnt);
                    }
                    //redirecionamento para o ficheiro output (por fazer)
                    if (i == n - 1)
                    {
                        redirect_output(f->number);
                    }
                    int count = words_count((f->commands)[i].command);
                    char **command_divided = malloc((count + 2) * sizeof(char *));
                    divide_command((f->commands)[i].command, command_divided);
                    (f->commands)[i].state = RUNNING;
                    (f->commands)[i].pid = getpid();
                    re_write_function(f);

                    int exec;
                    if ((exec = execvp(command_divided[0], command_divided)) < 0)
                    {
                        perror("Erro ao executar o comando");
                        _exit(status);
                    }

                    _exit(1);
                }
                wait(NULL);

                if (i < n - 1)
                    close(proxPipe[OUT]);
                if (i > 0)
                    close(pipeAnt);
                pipeAnt = proxPipe[IN];
            }
            _exit(FINISHED);
        }
        wait(&status);
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 2)
            {
                alarm(0);
                FUNCTION new = find_function(pid);
                new->state = FINISHED;
                re_write_function(new);
                end_output(new->number);
                free(new);
            }
            _exit(1);
        }
    }
    return 0;
}

/*
Função que lista as tarefas em execução
*/
int list(int pid_cliente)
{

    //encontrar o pipe pretendido (pipe+pid do cliente)
    char pid_string[32];
    sprintf(pid_string, "%d", pid_cliente);
    char nome_pipe[64] = "pipe";
    strcat(nome_pipe, pid_string);

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
            for (int i = 0; i < f->commands_number; i++)
            {
                char prog[COMMAND_LENGTH_MAX] = "";
                strcpy(prog, (f->commands[i]).command);
                write(out, prog, COMMAND_LENGTH_MAX);
                if (i < f->commands_number - 1)
                {
                    write(out, "|", 1);
                }
            }
            write(out, "\n", 1);
        }
    }

    sleep(1);
    write(out, "EOF", 3);
    close(fd);
    close(out);
    return 0;
}

/*
FUnção para mostrar ao cliente as tarefas já executadas ou terminadas
*/
int historico(int pid_cliente)
{
    //encontrar o pipe pretendido (pipe+pid do cliente)
    char pid_string[32];
    sprintf(pid_string, "%d", pid_cliente);
    char nome_pipe[64] = "pipe";
    strcat(nome_pipe, pid_string);

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
        if (f->type == EXECUTAR && (f->state == FINISHED || f->state == KILLED || f->state == TIMEEXEC || f->state == TIMEINAC))
        {
            write(out, "#", 1);
            char string[32] = "";
            sprintf(string, "%d", p);
            write(out, string, sizeof string);
            write(out, ", ", 2);

            if (f->state == KILLED)
            {
                write(out, "terminada", 9);
            }
            else if (f->state == FINISHED)
            {
                write(out, "concluida", 9);
            }
            else if (f->state == TIMEEXEC)
            {
                write(out, "max execução", 14);
            }
            else if (f->state == TIMEINAC)
            {
                write(out, "max inatividade", 15);
            }

            write(out, ": ", 2);
            for (int i = 0; i < f->commands_number; i++)
            {
                char prog[COMMAND_LENGTH_MAX] = "";
                strcpy(prog, (f->commands[i]).command);
                write(out, prog, COMMAND_LENGTH_MAX);
                if (i < f->commands_number - 1)
                {
                    write(out, "|", 1);
                }
            }
            write(out, "\n", 1);
        }
    }

    sleep(1);
    write(out, "EOF", 3);
    close(fd);
    close(out);
    return 0;
}

/*
Função dedicada à funcionalidade terminar.
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

    for (int i = 0; i < f->commands_number; i++)
    {
        if ((f->commands)[i].state == RUNNING)
        {
            kill((f->commands)[i].pid, SIGTERM);
        }
    }
    kill(pid, SIGTERM);

    close(fd);

    f->state = KILLED;
    re_write_function(f);

    free(f);

    return 1;
}

/*
Função que devolve o output pretendido ao cliente.
*/
int output(int pid_cliente, int line)
{
    printf("Pedido para aceder o output da tarefa: #%d\n", line);
    //encontrar o pipe pretendido (pipe+pid do cliente)
    char pid_string[32];
    sprintf(pid_string, "%d", pid_cliente);
    char nome_pipe[64] = "pipe";
    strcat(nome_pipe, pid_string);

    printf("Pedido de ouput, cliente: %d\n", pid_cliente);

    int fdidx;
    if ((fdidx = open(OUTPUT_INDEX, O_RDONLY, 0644)) < 0)
    {
        perror("error open index");
        return -1;
    }
    IDX index = malloc(sizeof(struct outputidx));

    //abrir o pipe com o cliente
    int out = open(nome_pipe, O_WRONLY);
    if (out < 0)
    {
        perror("Client offline");
        return -1;
    }

    //encontrar o indice
    int bytes_read;
    int find = 0;
    while ((bytes_read = read(fdidx, index, sizeof(struct outputidx))) > 0)
    {
        if (index->function_number == line)
        {
            find = 1;
            break;
        }
    }

    int size = 0;
    if (find == 1)
    {
        //buscar o output para o indice encontrado

        int fdout;
        if ((fdout = open(OUTPUT_FILE, O_RDONLY, 0644)) < 0)
        {
            perror("error open output");
            return -1;
        }

        lseek(fdout, index->offset, SEEK_SET);

        size = index->size;
        printf("ouput size:%d\n", size);
        if (size > 0)
        {
            char text[size];
            read(fdout, text, index->size);
            write(out, text, size);
        }
    }
    else
    {
        write(out, "Não foi possivel encontrar o output para a tarefa", 52);
    }

    sleep(1);
    write(out, "EOF", 3);
    close(fdidx);
    close(out);
    return 0;
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
            else if (f->type == LISTAR)
            {
                list(f->client);
            }
            else if (f->type == TERMINAR)
            {
                term(f->tarefa);
            }
            else if (f->type == HISTORICO)
            {
                historico(f->client);
            }
            else if (f->type == OUTPUT)
            {
                output(f->client, f->line);
            }
        }
        free(f);
    }

    close(in);

    return 0;
}
