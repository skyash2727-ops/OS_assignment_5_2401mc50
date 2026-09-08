// sem.h -- lightweight counting semaphores for Assignment 5 Q2/Q3/Q4.

#ifndef SEM_H
#define SEM_H

#define NSEM 64   // max number of semaphores system-wide

void seminit(void);
int  sem_alloc(int initial);
int  sem_wait(int id);
int  sem_post(int id);
int  sem_free(int id);

#endif
