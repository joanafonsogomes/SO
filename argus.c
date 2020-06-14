#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "argus.h"

/*
Função coloca na variavel recebida como argumento o nome do pipe
de onde o cliente recebe as mensagens vindas do servidor.
*/
void getpipe(char *pipe)
{
    int pid = getpid();
    char pid_string[32];
    sprintf(pid_string, "%d", pid);
    char fifo[64] = "pipe";
    strcat(fifo, pid_string);
    strcpy(pipe, fifo);
}

/*
Função que "apanha" os sigint's recebidos e fecha o cliente dando unlink ao pipe.
*/
void sigint_handler(int signum)
{
    puts("Closing client...");
    char fifo[64] = "";
    getpipe(fifo);
    unlink(fifo);
    _exit(0);
}

/*
Função similar ao puts do C que imprime para o ecrã (usando a chamada ao sistema write)
o char* passado como argumento.
*/
int myprint(char *s)
{
    const char *end = s;
    while (*end++)
        ;
    write(1, s, end - s - 1);
    return end - s - 1;
}

/*
Função que recebendo um char* como argumento separa-o no espaço 
e coloca o resultado no segundo argumento.
*/
int parse_linha(char *buff, char **str)
{
    char *tok;
    tok = strtok(buff, " ");

    str[0] = strdup(tok);
    tok = strtok(NULL, "");
    if (tok)
    {
        str[1] = strdup(tok);
    }
    return 1;
}

/*
Função para ler uma linha
*/
ssize_t readln(int fildes, void *buf, size_t bytes)
{
    size_t i = 0;
    ssize_t n = 1;
    char c = ' ';

    // Lê até bytes ou encontrar \n
    while ((i < bytes) && (n > 0) && (c != '\n'))
    {
        n = read(fildes, &c, 1);
        // coloca no buf enquanto não é \n
        if (n && (c != '\n'))
        {
            ((char *)buf)[i] = c;
            i++;
        }
    }

    // caso chegue ao fim do buf
    if (i < bytes)
    {
        ((char *)buf)[i] = 0;
    }
    else
    {
        i--;
        ((char *)buf)[i] = 0;
    }

    
    if (n < 0)
    {
        return n;
    }
    // encontra só \n
    if ((n == 0) && (i == 0))
    {
        return (-1);
    }
    return i;
}

/*
Função que envia através do pipe a estrutura FUNCTION passada como argumento.
*/
int send(FUNCTION new_function)
{
    int out = open(SERVER_PIPE, O_WRONLY);
    if (out < 0)
    {
        puts("Server's offline");
        return -1;
    }
    new_function->client = getpid();

    int res;
    if ((res = write(out, new_function, sizeof(struct function))) < 0)
    {
        perror("Error in write in pipe");
    };

    close(out);
    return 1;
}


/*
Função que conta quantos comandos foram escritos no ficheiro auxiliar.
*/
int commands_number()
{
    COMMAND c = malloc(sizeof(struct command));
    int bytes_read;
    int commands_number = 0;
    int fd = open(COMMAND_FILE, O_RDWR, 0640);
    while ((bytes_read = read(fd, c, sizeof(struct command))) > 0)
    {
        commands_number++;
    }
    close(fd);
    free(c);
    return commands_number;
}

/*
Função que inicializa a estrutura FUNCTION, coloca todos os comandos e envia para o servidor.
*/
int send_server_exec()
{
    int c_number = commands_number();
    int bytes_read;
    FUNCTION new_function = malloc(sizeof(struct function));
    new_function->type = EXECUTAR;
    new_function->commands_number = c_number;

    int fd1 = open(COMMAND_FILE, O_RDWR, 0640);

    for (int i = 0; i < c_number; i++)
    {
        struct command c;

        if ((bytes_read = read(fd1, &c, sizeof(struct command))) > 0)
        {
            (new_function->commands)[i] = c;
        }
        else
        {
            perror("Error in read");
        }
    }
    close(fd1);
    unlink(COMMAND_FILE);

    send(new_function);

    free(new_function);
    return 0;
}

/*
Função que escreve um comando passado como argumento 
para o ficheiro que armazena temporariamente os comandos
*/
int write_command(int fd, char *command)
{

    int res;
    COMMAND new_command = malloc(sizeof(struct command));
    new_command->state = INITIAL;
    strcpy(new_command->command, command);

    if ((res = write(fd, new_command, sizeof(struct command))) < 0)
    {
        myprint("Error in write\n");
    };

    return res;
}

/*
Função que faz parse dos comandos recebidos como argumento, 
retirando as aspas no caso da shell e separando por '|'.
*/
int parse_arg(char *buff, int j)
{
    int i;
    char *tok;
    int fd = open(COMMAND_FILE, O_RDWR | O_CREAT, 0640);
    lseek(fd, 0, SEEK_END);
    char *str2 = buff;

    //retirar aspas na shell
    if (j == SHELL)
    {
        //aponta para o segundo elemento do buffer
        str2 = &(buff[1]);
        //retira o ultimo elemento da string
        str2[strlen(str2) - 1] = '\0';
    }

    tok = strtok(str2, "|");

    for (i = 0; tok; i++)
    {
        write_command(fd, strdup(tok));
        tok = strtok(NULL, "|");
    }
    close(fd);
    return i;
}

/*
Funcão dedicada à funcionalidade executar
*/
void exec(char *args, int i)
{
    parse_arg(args, i);
    send_server_exec();

    char pipe[64];
    getpipe(pipe);
    int in;
    if ((in = open(pipe, O_RDONLY)) < 0)
    {
        perror("open() in");
    }

    char buff[512] = "";
    readln(in, buff, 512);
    write(1, buff, 512);

    close(in);
}

/*
Funcão dedicada à funcionalidade tempo de execução
*/
void tmp_exec(char *args)
{
    // myprint(args);
    FUNCTION new_function = malloc(sizeof(struct function));
    new_function->type = TEMPO_EXECUCAO;
    new_function->tempo = atoi(args);
    send(new_function);
    free(new_function);
}

/*
Funcão dedicada à funcionalidade tempo de inatividade em pipe
*/
void tmp_inat(char *args)
{
    // myprint(args);
    FUNCTION new_function = malloc(sizeof(struct function));
    new_function->type = TEMPO_INATIVIDADE;
    new_function->tempo = atoi(args);
    send(new_function);
    free(new_function);
}

/*
Função para a funcionalidade listar
*/
int list()
{
    FUNCTION new_function = malloc(sizeof(struct function));
    new_function->type = LISTAR;
    new_function->client = getpid();
    send(new_function);

    char pipe[64];
    getpipe(pipe);
    int in;
    if ((in = open(pipe, O_RDONLY)) < 0)
    {
        perror("open() in");
        return -1;
    }

    char buf[512];
    int n;
    while (1)
    {
        n = read(in, buf, 512);
        if (n < 0)
            ;
        else if (strncmp(buf, "EOF", 3) == 0)
            break;
        else
        {
            write(1, buf, n);
        }
    }

    free(new_function);
    close(in);
    return 1;
}

/*
Função para a funcionalidade terminar tarefa
*/
int term(int number)
{
    FUNCTION f = malloc(sizeof(struct function));
    f->type = TERMINAR;
    f->tarefa = number;
    send(f);
    free(f);
    return 1;
}

/*
Função para a funcionalidade historico
*/
int hist()
{
    FUNCTION new_function = malloc(sizeof(struct function));
    new_function->type = HISTORICO;
    new_function->client = getpid();
    send(new_function);

    char pipe[64];
    getpipe(pipe);
    int in;
    if ((in = open(pipe, O_RDONLY)) < 0)
    {
        perror("open() in");
        return -1;
    }

    char buf[512];
    int n;
    while (1)
    {
        n = read(in, buf, 512);
        if (n < 0)
            ;
        else if (strncmp(buf, "EOF", 3) == 0)
            break;
        else
        {
            write(1, buf, n);
        }
    }

    free(new_function);
    close(in);
    return 1;
}

/*
Função para a funcionalidade output
*/
int output(int line)
{
    FUNCTION f = malloc(sizeof(struct function));
    f->type = OUTPUT;
    f->line = line;
    send(f);
    free(f);

    char pipe[64];
    getpipe(pipe);
    int in;
    if ((in = open(pipe, O_RDONLY)) < 0)
    {
        perror("open() in");
        return -1;
    }

    char buf[512];
    int n;
    while (1)
    {
        n = read(in, buf, 512);
        if (n < 0)
            ;
        else if (strncmp(buf, "EOF", 3) == 0)
            break;
        else
        {
            write(1, buf, n);
        }
    }

    close(in);

    return 1;
}

/*
Função para executar as funcionalidades em modo shell
*/
int shell()
{
    char *buff = malloc(sizeof(char *) * 150);
    while (1)
    {
        myprint("argus$ ");
        if (readln(0, buff, sizeof(char *) * 150))
        {
            printf("-> %s\n", buff);
            char **args = malloc(sizeof(char **));
            parse_linha(buff, args);
            if (!strcmp(args[0], "tempo-inactividade") && args[1])
            {
                //tempo inatividade
                tmp_inat(args[1]);
            }
            else if (!strcmp(args[0], "tempo-execucao") && args[1])
            {
                //tempo execucao
                tmp_exec(args[1]);
            }
            else if (!strcmp(args[0], "executar") && args[1])
            {
                //exec
                //myprint(args[1]);
                exec(args[1], SHELL);
            }
            else if (!strcmp(args[0], "listar"))
            {
                //list
                list();
            }
            else if (!strcmp(args[0], "terminar") && args[1])
            {
                //kill
                term(atoi(args[1]));
            }
            else if (!strcmp(args[0], "historico"))
            {
                //history
                hist();
            }
            else if (!strcmp(args[0], "output") && args[1])
            {
                //history
                output(atoi(args[1]));
            }
            else if (!strcmp(args[0], "exit"))
            {
                break;
            }
            else if (!strcmp(args[0], "ajuda"))
            {

                myprint("\e[1mtempo-inactividade\e[0m segs\n");
                myprint("\e[1mtempo-execucao\e[0m segs\n");
                myprint("\e[1mexecutar\e[0m \"p1 | p2 ... | pn\"\n");
                myprint("\e[1mlistar\e[0m\n");
                myprint("\e[1mterminar\e[0m tarefa\n");
                myprint("\e[1mhistorico\e[0m\n");
                myprint("\e[1moutput\e[0m tarefa\n");
            }
            else
            {
                myprint("Comando invalido! Insira \e[4majuda\e[24m para obter ajuda.\e[0m");
            }
            free(args);
        }
        else
        {
            myprint("\e[1mComando invalido! Insira \e[4majuda\e[24m para obter ajuda.\e[0m");
        }
        myprint("\n");
    }
    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    //criar o fifo para receber a informação
    char pipe[64];
    getpipe(pipe);
    mkfifo(pipe, 0666);

    if (argc == 1)
    {
        return shell();
    }
    else
    {
        if (!strcmp(argv[1], "-i") && argv[2] && !argv[3])
        {
            //tempo-inatividade
            tmp_inat(argv[2]);
        }
        else if (!strcmp(argv[1], "-m") && argv[2] && !argv[3])
        {
            //tempo execucao
            tmp_exec(argv[2]);
        }
        else if (!strcmp(argv[1], "-e") && argv[2] && !argv[3])
        {

            exec(argv[2], TERMINAL);
        }
        else if (!strcmp(argv[1], "-l") && !argv[2])
        {
            //list))
            list();
        }
        else if (!strcmp(argv[1], "-t") && argv[2] && !argv[3])
        {
            //kill
            term(atoi(argv[2]));
        }
        else if (!strcmp(argv[1], "-r") && !argv[2])
        {
            //history
            hist();
        }
        else if (!strcmp(argv[1], "-o") && argv[2] && !argv[3])
        {
            //history
            output(atoi(argv[2]));
        }
        else if (!strcmp(argv[1], "-h"))
        {
            //help))
            myprint("\e[4m\e[1mAJUDA\e[0m\e[24m\n\n");
            myprint("Tempo de inatividade \n     ./argus \e[1m-i\e[0m segs\n\n");
            myprint("Tempo de execução \n     ./argus \e[1m-m\e[0m segs\n\n");
            myprint("Executar\n      ./argus \e[1m-e\e[0m \"p1 | p2 ... | pn\"\n\n");
            myprint("Listar\n     ./argus \e[1m-l\e[0m\n\n");
            myprint("Terminar uma tarefa\n     ./argus \e[1m-t\e[0m tarefa\n\n");
            myprint("Histório\n     ./argus \e[1m-r\e[0m \n\n");
            myprint("Output de uma tarefa\n     ./argus \e[1m-o\e[0m tarefa\n");
        }
        else
        {
            myprint("\e[1mComando invalido! Insira ./argus \e[4m-h\e[24m para obter ajuda.\e[0m");
        }
        printf("\n");
    }
    unlink(pipe);

    return 0;
}
