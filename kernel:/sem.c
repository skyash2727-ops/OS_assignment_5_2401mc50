// sem.c -- Assignment 5: our own counting semaphore, implemented in
// the kernel on top of xv6's existing sleep()/wakeup() primitives
// (the same mechanism sleeplock.c uses), and exposed to user
// programs via four new syscalls: sem_alloc, sem_wait, sem_post,
// sem_free.
//
// Semaphores live in a fixed-size global table indexed by a small
// integer id. Because the table is part of the kernel (not any one
// process's address space), unrelated processes can share a
// semaphore just by agreeing on its id -- which is exactly what
// happens here: a parent calls sem_alloc() before fork(), and the
// resulting id is a plain integer that fork() copies into the
// child's stack/data like any other local variable, so both
// processes end up referring to the same kernel semaphore object.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sem.h"

struct sem {
  struct spinlock lock;
  int value;     // current semaphore value (>=0 means available)
  int inuse;     // slot allocated?
};

static struct sem semtable[NSEM];

void
seminit(void)
{
  int i;

  for(i = 0; i < NSEM; i++){
    initlock(&semtable[i].lock, "sem");
    semtable[i].value = 0;
    semtable[i].inuse = 0;
  }
}

// Allocate a free semaphore slot with the given initial value.
// Returns its id (>=0) or -1 if the table is full.
int
sem_alloc(int initial)
{
  int i;

  for(i = 0; i < NSEM; i++){
    acquire(&semtable[i].lock);
    if(!semtable[i].inuse){
      semtable[i].inuse = 1;
      semtable[i].value = initial;
      release(&semtable[i].lock);
      return i;
    }
    release(&semtable[i].lock);
  }
  return -1;
}

// P(): block (put the calling process to SLEEPING) while the
// semaphore's value is 0, then decrement it.
int
sem_wait(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;

  acquire(&semtable[id].lock);
  if(!semtable[id].inuse){
    release(&semtable[id].lock);
    return -1;
  }
  while(semtable[id].value <= 0){
    // sleep() atomically releases semtable[id].lock and reacquires
    // it once woken up by a matching sem_post().
    sleep(&semtable[id], &semtable[id].lock);
  }
  semtable[id].value--;
  release(&semtable[id].lock);
  return 0;
}

// V(): increment the semaphore's value and wake up anyone waiting.
int
sem_post(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;

  acquire(&semtable[id].lock);
  if(!semtable[id].inuse){
    release(&semtable[id].lock);
    return -1;
  }
  semtable[id].value++;
  wakeup(&semtable[id]);
  release(&semtable[id].lock);
  return 0;
}

// Release a semaphore slot so it can be reused.
int
sem_free(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;

  acquire(&semtable[id].lock);
  semtable[id].inuse = 0;
  semtable[id].value = 0;
  release(&semtable[id].lock);
  return 0;
}
