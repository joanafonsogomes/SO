#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "argus.h"

int myprint(char *s)
{
    const char *end = s;
    while (*end++)
        ;
    write(1, s, end - s - 1);
    return end - s - 1;
}

//Função que faz parsing de uma linha
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

int command_finish(char *command)
{
    COMMAND c = malloc(sizeof(struct command));
    int bytes_read;
    int res = -1;

    myprint("ola tudo bem?\n");
    int fd = open(COMMAND_FILE, O_RDWR, 0640);
    myprint("nao xau\n");
    while ((bytes_read = read(fd, c, sizeof(struct command))) > 0)
    {
        myprint(c->command);
        if (strcmp(c->command, command))
        {
            c->state = FINISHED;
            res = lseek(fd, -sizeof(struct command), SEEK_CUR);
            res = write(fd, &c, sizeof(struct command));
        }
        else
        {
            lseek(fd, sizeof(struct command), SEEK_CUR);
        }
        myprint("\n");
        printf("state: %d\n",c->state);
    }

    return res;
}


//Escrever um comando passado como argumento para o ficheiro que contem todos os comandos usando o append
int write_command(char *command)
{
    int res;
    COMMAND new_command = malloc(sizeof(struct command));
    new_command->state = RUNNING;
    strcpy(new_command->command, command);

    int fd = open(COMMAND_FILE, O_RDWR | O_APPEND | O_CREAT, 0640);

    if((res = write(fd, &new_command, sizeof(struct command)))<=0){
        myprint("Error in write\n");
    };
    close(fd);
    return res;
}

//Função que faz parsing do argumento
int parse_arg(char *buff)
{
    int i;
    char *tok;

    //aponta para o segundo elemento do buffer
    char *str2 = &(buff[1]);
    //retira o ultimo elemento da string
    str2[strlen(str2) - 1] = '\0';

    tok = strtok(str2, "|");

    myprint(tok);
    myprint("\n");

    for (i = 0; tok; i++)
    {
        write_command(strdup(tok));
        tok = strtok(NULL, "|");
        myprint(tok);
        myprint("\n");
        i = command_finish(tok);
        myprint("\n");
    }
    myprint("acabou\n");
    return i;
}

ssize_t readln(int fd, void *buff, size_t n)
{
    char c = ' ';
    size_t s = 0, r = 1;
    char *temp = (char *)buff;

    while ((s < n) && r && (c != '\n'))
    {

        r = read(fd, &c, 1);

        if (r && (c != '\n'))
        {
            temp[s] = c;
            s++;
        }
    }
    temp[s] = 0;
    if (s == 0 && temp[s] == '\n')
        return -1;

    return s;
}

void exec(char *args)
{
    parse_arg(args);
}

int shell()
{
    char *buff = malloc(sizeof(char*)*150);
    while (1)
    {
        myprint("argus$ ");
        if (readln(0, buff, sizeof(char*)*150))
        {

            char **args = malloc(sizeof(char **));
            parse_linha(buff, args);
            if (!strcmp(args[0], "tempo-inactividade") && args[1])
            {
                //tempo inatividade
                myprint("inac\n");
            }
            else if (!strcmp(args[0], "tempo-execucao") && args[1])
            {
                //tempo execucao
                myprint("texec\n");
            }
            else if (!strcmp(args[0], "executar") && args[1])
            {
                //exec
                //myprint(args[1]);
                exec(args[1]);
            }
            else if (!strcmp(args[0], "listar"))
            {
                //list
                myprint("list\n");
            }
            else if (!strcmp(args[0], "terminar") && args[1])
            {
                //kill
                myprint("term\n");
            }
            else if (!strcmp(args[0], "historico"))
            {
                //history
                myprint("his\n");
            }
            else if (!strcmp(args[0], "ajuda"))
            {

                myprint("\e[1mtempo-inactividade\e[0m segs\n");
                myprint("\e[1mtempo-execucao\e[0m segs\n");
                myprint("\e[1mexecutar\e[0m p1 | p2 ... | pn\n");
                myprint("\e[1mlistar\e[0m\n");
                myprint("\e[1mterminar\e[0m tarefa\n");
                myprint("\e[1mhistorico\e[0m\n");
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
    if (argc == 1)
    {
        return shell();
    }
    else
    {
        if (!strcmp(argv[1], "-i"))
        {
            //tempo-inatividade
            myprint("inac\n");
        }
        else if (!strcmp(argv[1], "-m"))
        {
            //tempo execucao
            myprint("texec\n");
        }
        else if (!strcmp(argv[1], "-e"))
        {
            //exec
            myprint("exec\n");
        }
        else if (!strcmp(argv[1], "-l"))
        {
            //list))
            myprint("list\n");
        }
        else if (!strcmp(argv[1], "-t"))
        {
            //kill
            myprint("term\n");
        }
        else if (!strcmp(argv[1], "-r"))
        {
            //history
            myprint("his\n");
        }
        else if (!strcmp(argv[1], "-h"))
        {
            //help))
            myprint("\e[1margus\e[0m segs\n");
            myprint("\e[1m-i\e[0m segs\n");
            myprint("\e[1m-m\e[0m segs\n");
            myprint("\e[1m-e\e[0m p1 | p2 ... | pn\n");
            myprint("\e[1m-l\e[0m\n");
            myprint("\e[1m-t\e[0m tarefa\n");
            myprint("\e[1m-r\e[0m\n");
        }
        else
        {
            myprint("\e[1mComando invalido! Insira \e[4majuda\e[24m para obter ajuda.\e[0m");
        }
        printf("\n");
    }

    return 0;
}
