// peterson.c -- Assignment 5, Question 1
//
// Mutual exclusion between a parent and one child process using
// Peterson's Algorithm, enforced ONLY with the flag[]/turn variables
// living in a page of memory shared between the two processes
// (obtained through the new shm_get() syscall). No xv6 lock is used
// to protect the critical section itself.

#include "types.h"
#include "stat.h"
#include "user.h"

#define ITERS 10          // iterations per process
#define OTHER(id) (1 - (id))

// Everything the two processes need to agree on lives in this
// struct, which is placed on the single page returned by shm_get().
//
// IMPORTANT: every field here is read by a DIFFERENT process than
// the one that wrote it, purely through the busy-wait loop below --
// there's no function call (like sem_wait/sem_post) in between to
// act as a compiler barrier. Without `volatile`, gcc -O2 is free to
// load s->flag[other]/s->turn into a register once, before the loop,
// and then spin on that stale cached value forever, since nothing
// *it can see* ever changes them. That turns the busy-wait into a
// genuine infinite loop -- which is exactly the hang you get if you
// drop the volatile qualifiers here.
struct shared {
  volatile int flag[2];         // flag[i]: process i wants to enter the CS
  volatile int turn;             // whose turn it is when both want in
  volatile int shared_counter;   // counter incremented inside the CS
};

// Busy loop used as the "remainder section" so output is readable
// and interleaving is actually exercised instead of one process
// racing through all 10 iterations before the other gets scheduled.
static void
delay(int n)
{
  volatile int i;
  for(i = 0; i < n; i++)
    ;
}

static void
run(volatile struct shared *s, int id)
{
  int i;

  for(i = 0; i < ITERS; i++){
    // ---- entry section (pure Peterson, no OS lock) ----
    s->flag[id] = 1;
    s->turn = OTHER(id);
    while(s->flag[OTHER(id)] == 1 && s->turn == OTHER(id))
      ;  // busy-wait -- must re-read shared memory every iteration

    // ---- critical section ----
    s->shared_counter = s->shared_counter + 1;
    printf(1, "Process %d in CS, counter = %d\n", id, s->shared_counter);

    // ---- exit section ----
    s->flag[id] = 0;

    // ---- remainder section ----
    delay(300000);
  }
}

int
main(void)
{
  int shmva;
  volatile struct shared *s;
  int pid;

  shmva = shm_get();
  if(shmva < 0){
    printf(1, "peterson: shm_get failed\n");
    exit();
  }
  s = (volatile struct shared*)shmva;

  // Initialize shared state before the child comes along.
  s->flag[0] = 0;
  s->flag[1] = 0;
  s->turn = 0;
  s->shared_counter = 0;

  pid = fork();
  if(pid < 0){
    printf(1, "peterson: fork failed\n");
    exit();
  }

  if(pid == 0){
    // Child: fork() gave it a private copy of the address space up
    // to the process size, so the shm page (mapped far above that)
    // is NOT present in the child yet. Map it again -- shm_get()
    // hands back the SAME physical page, so both processes now
    // truly share s->flag[]/s->turn/s->shared_counter.
    shmva = shm_get();
    s = (volatile struct shared*)shmva;
    run(s, 1);
    exit();
  } else {
    run(s, 0);
    wait();
    printf(1, "peterson: done, final counter = %d (expected %d)\n",
           s->shared_counter, 2 * ITERS);
  }

  exit();
}