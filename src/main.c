#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "imprimer.h"
#include <string.h>
#include "extra.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
/*
 * "Imprimer" printer spooler.
 */
static int imp_num_printers;
static PRINTER printers[MAX_PRINTERS];
static TYPE *head;
static Q *front, *end;
static int jobCount;
static char *func;

int main(int argc, char *argv[])
{
        char optval;
        FILE *in = NULL;
        FILE *out = NULL;
        if(argc > 1)
        {
            while(optind < argc) {
                if((optval = getopt(argc, argv, "i:o:")) != -1) {
                    switch(optval) {
                        case 'i':
                            in = fopen(optarg, "r");
                            if(in == NULL)
                            {
                                char *buf = malloc(1024 + strlen(optarg));
                                printf("%s\n", imp_format_error_message("Input file could not be accessed.", buf, 75));
                                free(buf);
                                exit(EXIT_FAILURE);
                            }
                            dup2(fileno(in), 0);
                            break;
                        case 'o':
                            out = fopen(optarg, "w");
                            if(out == NULL)
                            {
                                char *buf = malloc(1024 + strlen(optarg));
                                printf("%s\n", imp_format_error_message("Output file could not be accessed.", buf, 75));
                                free(buf);
                                exit(EXIT_FAILURE);
                            }
                            dup2(fileno(out), 1);
                            break;
                        case '?':
                            fprintf(stderr, "Usage: %s [-i <cmd_file>] [-o <out_file>]\n", argv[0]);
                            exit(EXIT_FAILURE);
                        break;
                        default:
                        break;
                    }
                }
            }
        }
    front = malloc(sizeof(Q));
    end = front;
    end->next = NULL;
    head = NULL;

    imp_num_printers = 0;
    do{
        char *d = readline("imp>");
        char *c = malloc(strlen(d) + 1);
        memcpy(c, d, strlen(d) + 1);

        free(d);
        char *tok = strtok(c, " ");
        if(tok == NULL)
        {
            char *buf = malloc(1024);
            printf("%s\n", imp_format_error_message("No command entered.", buf, 64));
            free(buf);
            free(c);
            continue;
        }

        if(strcmp(tok, "quit") == 0)
        {
            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            TYPE *temp = head;
            while(temp != NULL)
            {
                struct conv *conv = temp->head;
                while(conv != NULL)
                {
                    struct conv *ct = conv;
                    conv = conv->next;
                    free(ct->function);
                    free(ct);
                }
                TYPE *t = temp;
                temp = temp->next;
                free(t->name);
                free(t);
            }
            free(c);

            for(int i = 0; i < imp_num_printers; i++)
            {
                PRINTER *p = &printers[i];
                free(p->name);
                free(p->type);
            }

            while(front->job != NULL)
            {
                Q *t = front;

                free(t->job->file_name);
                free(t->job->file_type);
                free(t->job);
                front = front->next;
                free(t);
            }
            int i = 0;
            while(wait(&i) >= 0)
            {
                continue;
            }
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(tok, "help") == 0)
        {
            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            printf("Commands\n");
            printf("\thelp\t Display possible commands.\n");
            printf("\tquit\t Exit the program.\n");
            printf("\ttype file_type\t Declares a file_type to be a file type supported by the imprimer.\n");
            printf("\tprinter printer_name file_type\t Creates a printer named printer_name that can print files of type file_type.\n");
            printf("\tconversion file_type1 file_type2 conversion_program [arg0 arg1 ...]\t Declares files of type file_type1 can be converted to type file_type2 using the program conversion_program and arguments [arg0 arg1 ...].\n");
            printf("\tprinters\t Displays the current status of all declared printers.\n");
            printf("\tjobs\t Displays the status of all queued print jobs.\n");
            printf("\tprint file_name [printer_name1 printer_name2 ...]\t Sets up a job to print the file file_name. Optional arguments [printer_name1 printer_name2 ...] may be given to specify valid declared printers to send the job to.\n");
            printf("\tcancel job_number\t Cancels the existing job with job number job_number.\n");
            printf("\tpause job_number\t Pauses the existing job with job number job_number.\n");
            printf("\tresume job_number\t Resumes the existing, paused job with job number job_number.\n");
            printf("\tdisable printer_name\t Prevents the printer named printer_name from processing any new jobs until it is re-enabled.\n");
            printf("\tenable printer_name\t Allows the printer named printer_name to start processing jobs.\n");
        }
        else if(strcmp(tok, "type") == 0)
        {
            char *tok = strtok(NULL, " ");
            if(tok == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            char *t;
            if((t = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            // Probably will have a String array of all types
            TYPE *temp = head;
            int flag = 1;
            while(temp != NULL)
            {
                if(strcmp(temp->name, tok) == 0)
                {
                    char *buf = malloc(1024 + strlen(tok));
                    char *s = strcat(tok, " is already defined as a file type.");
                    printf("%s\n", imp_format_error_message(s, buf, 55 + strlen(tok)));
                    free(buf);
                    flag = 0;
                    break;
                }
                temp = temp->next;
            }
            if(flag)
            {
                temp = malloc(sizeof(TYPE));
                temp->name = tok;
                temp->head = NULL;
                temp->next = head;
                head = temp;
            }
        }
        else if(strcmp(tok, "printer") == 0)
        {
            // check validity of type first with loop through String array
            if(imp_num_printers >= MAX_PRINTERS)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("The maximum number of printers have been made.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            char *name = strtok(NULL, " ");
            if(name == NULL)
            {
                free(c);
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                continue;
            }

            char *type = strtok(NULL, " ");

            if(type == NULL)
            {
                free(c);
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                continue;
            }
            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            TYPE *temp = head;
            int flag = 1;
            while(temp != NULL)
            {
                if(strcmp(temp->name, type) == 0)
                {
                    flag = 0;
                    break;
                }
                temp = temp->next;
            }
            if(flag)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("The specified file type has not been defined.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            PRINTER *p = &(printers[imp_num_printers]);
            p->id = imp_num_printers;
            p->name = malloc(strlen(name) + 1);
            p->type = malloc(strlen(type) + 1);
            memcpy(p->name, name, strlen(name) + 1);
            memcpy(p->type, type, strlen(type) + 1);
            p->enabled = 0;
            p->busy = 0;
            imp_num_printers++;
        }
        else if(strcmp(tok, "conversion") == 0)
        {
            char *type1 = strtok(NULL, " ");
            if(type1 == NULL)
            {
                free(c);
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                continue;
            }
            char *type2 = strtok(NULL, " ");
            if(type2 == NULL)
            {
                free(c);
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                continue;
            }
            TYPE *src = head;
            while(src != NULL)
            {
                if(strcmp(src->name, type1) == 0)
                {
                    break;
                }
                src = src->next;
            }
            if(src == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("The first file type has not been defined.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            TYPE *dest = head;
            while(dest != NULL)
            {
                if(strcmp(dest->name, type2) == 0)
                {
                    break;
                }
                dest = dest->next;
            }
            if(dest == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("The second file type has not been defined.", buf, 75));
                free(buf);
                free(c);
                continue;
            }

            struct conv *prog= malloc(sizeof(struct conv));
            char *ex = strtok(NULL, " ");
            if(ex == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(prog);
                free(c);
                continue;
            }
            prog->function[0] = malloc(strlen(ex) + 1);
            memcpy(prog->function[0], ex, strlen(ex)+1);
            int count = 1;
            while((ex = strtok(NULL, " ")) != NULL)
            {
                prog->function[count] = malloc(strlen(ex) + 1);
                memcpy(prog->function[count], ex, strlen(ex) + 1);
                count++;
            }
            prog->function[count] = (char *)NULL;
            prog->f = dest;
            prog->next = src->head;
            src->head = prog;
        }
        else if(strcmp(tok, "printers") == 0)
        {
            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            int flag = 1;
            for(int i = 0; i < imp_num_printers; i++)
            {
                flag = 0;
                char *buf = malloc(1024 + strlen(printers[i].name) + 1);
                printf("%s\n", imp_format_printer_status(&printers[i], buf, 1024 + strlen(printers[i].name) + 1));
                free(buf);
            }
            if(flag)
            {
                printf("There are currently no defined printers.\n");
            }
        }
        else if(strcmp(tok, "jobs") == 0)
        {
            if((void *)front == (void *)end)
            {
                printf("The are no jobs in the queue.\n");
                free(c);
                continue;
            }
            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            Q *temp = front;
            while((void *)temp != (void *)end)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_job_status(temp->job, buf, 1024));
                free(buf);
                temp = temp->next;
            }
        }
        else if(strcmp(tok, "print") == 0)
        {
            char *name = strtok(NULL, " ");
            if(name == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            JOB *j = malloc(sizeof(JOB));
            j->file_name = malloc(strlen(name) + 1);
            memcpy(j->file_name, name, strlen(name) + 1);
            TYPE *temp = head;
            int typeFlag = 1;
            while(temp != NULL)
            {
                if(strstr(name, temp->name))
                {
                    typeFlag = 0;
                    j->file_type = malloc(strlen(temp->name) + 1);
                    memcpy(j->file_type, temp->name, strlen(temp->name) + 1);
                    break;
                }
                temp = temp->next;
            }

            PRINTER_SET set = 0;
            char *printer  = strtok(NULL, " ");
            int mainFlag = 0;
            if(printer == NULL)
            {
                set = ANY_PRINTER;
            }
            else
            {
                while(printer != NULL)
                {
                    int flag = 1;
                    for(int i = 0; i < imp_num_printers; i++)
                    {
                        if(strcmp(printers[i].name, printer) == 0)
                        {
                            flag = 0;
                            set = set & (0x1 << printers[i].id);
                            break;
                        }
                    }
                    if(flag)
                    {
                        mainFlag = 1;
                        char *buf = malloc(1024 + strlen(printer));
                        char *hold = malloc(strlen(printer) + 1024);
                        memcpy(hold, printer, strlen(printer) + 1);
                        printf("%s\n", imp_format_error_message(strcat(hold, " is not a defined printer."), buf, 75));
                        free(hold);
                        free(buf);
                        break;
                    }
                    printer = strtok(NULL, " ");
                }
            }
            if(mainFlag)
            {
                free(j->file_name);
                free(j);
                free(c);
                continue;
            }
            j->eligible_printers = set;
            j->jobid = jobCount++;
            j->status = QUEUED;
            gettimeofday(&(j->creation_time), NULL);

            end->job = j;
            end->next = malloc(sizeof(Q));
            end->isDefined = typeFlag;
            end = end->next;
            end->next = NULL;
        }
        else if(strcmp(tok, "disable") == 0)
        {
            char *name = strtok(NULL, " ");
            if(name == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            for(int i = 0; i < imp_num_printers; i++)
            {
                if(strcmp(printers[i].name, name) == 0)
                {
                    printers[i].enabled = 0;
                    char *buf = malloc(1024 + strlen(printers[i].name) + 1);
                    printf("%s\n", imp_format_printer_status(&printers[i], buf, 1024 + strlen(printers[i].name) + 1));
                    free(buf);
                    break;
                }
            }
        }
        else if(strcmp(tok, "enable") == 0)
        {
            char *name = strtok(NULL, " ");
            if(name == NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To few arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
                            if((tok = strtok(NULL, " ")) != NULL)
            {
                char *buf = malloc(1024);
                printf("%s\n", imp_format_error_message("To many arguments. Use \"help\" for usages.", buf, 75));
                free(buf);
                free(c);
                continue;
            }
            for(int i = 0; i < imp_num_printers; i++)
            {
                if(strcmp(printers[i].name, name) == 0)
                {
                    printers[i].enabled = 0;
                    char *buf = malloc(1024 + strlen(printers[i].name) + 1);
                    printf("%s\n", imp_format_printer_status(&printers[i], buf, 1024 + strlen(printers[i].name) + 1));
                    free(buf);
                    break;
                }
            }
        }
        else
        {
            free(c);
            char *buf = malloc(1024);
            printf("%s\n", imp_format_error_message("That is an invalid input. Use \"help\" to list valid commands.", buf, 75));
            free(buf);
        }
   // void *d = c;
   // d -= offsetLength;

    }while(1);

    exit(EXIT_SUCCESS);
}

int findconv(TYPE *src, TYPE *dest, char *visited)
{
    char *buf = malloc(strlen(visited) + strlen(src->name) + 2);
    strcpy(buf, visited);
    strcat(buf, src->name);
    strcat(buf, " ");
    struct conv *c = src->head;
    while(c != NULL)
    {
        if((void *)(c->f) == (void * )dest)
        {

            func = malloc(strlen(buf) + strlen(c->f->name) + 1);
            strcpy(func, buf);
            strcat(func, c->f->name);
            free(buf);
            return 1;
        }
        c = c->next;
    }
    c = src->head;
    while(c != NULL)
    {
        if(!strstr(c->f->name, buf))
        {
            if(findconv(c->f, dest, buf))
            {
                free(buf);
                return 1;
            }
        }
        c = c->next;
    }
    free(buf);
    return 0;
}
