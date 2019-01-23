#ifndef EXTRA_H
#define EXTRA_H


typedef struct fileType
{
    char *name;
    struct conv{
        struct fileType *f;
        char *function[30];
        struct conv *next;} *head;
    struct fileType *next; // next linked list
} TYPE;

typedef struct jobQueue
{
    JOB *job;
    struct jobQueue *next;
    int isDefined;
} Q;
#endif