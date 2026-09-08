// dining.c -- Assignment 5, Question 4
//
// 5 philosophers, 5 forks. Each fork is a binary semaphore (sem.c),
// allocated once by the parent before forking, so every philosopher
// process inherits (via fork()'s copy of the data segment) the same
// 5 semaphore ids, which all refer to the SAME kernel semaphore
// objects. No shared memory page is needed here: each philosopher
// only needs to know the ids of its own two forks, and all output
// goes straight to the console, which already serializes writes
// from different processes.
//
// Deadlock-avoidance strategy: ASYMMETRIC fork ordering.
//   - Even-numbered philosophers pick up their LEFT fork first, then
//     their RIGHT fork.
//   - Odd-numbered philosophers pick up their RIGHT fork first, then
//     their LEFT fork.
// With 5 (odd) philosophers this breaks the circular-wait condition:
// it is impossible for every philosopher to be simultaneously
// holding one fork and waiting on the same-handed neighbour, so the
// "all philosophers holding their left fork, waiting for their
// right" deadlock cannot occur.

#include "types.h"
#include "stat.h"
#include "user.h"

#define NPHIL 5
#define CYCLES 5   // configurable number of think/eat cycles

int forks[NPHIL];  // semaphore id for fork i

static void
sleep_rand(int base, int id)
{
  sleep(base + (id % 3));
}

static void
philosopher(int id)
{
  int left, right, c;

  left = id;
  right = (id + 1) % NPHIL;

  for(c = 0; c < CYCLES; c++){
    printf(1, "Philosopher %d: THINKING\n", id);
    sleep_rand(3, id);

    printf(1, "Philosopher %d: THINKING -> HUNGRY\n", id);

    if(id % 2 == 0){
      // even: left fork first, then right
      sem_wait(forks[left]);
      sem_wait(forks[right]);
    } else {
      // odd: right fork first, then left
      sem_wait(forks[right]);
      sem_wait(forks[left]);
    }

    printf(1, "Philosopher %d: HUNGRY -> EATING\n", id);
    sleep_rand(2, id);

    sem_post(forks[left]);
    sem_post(forks[right]);

    printf(1, "Philosopher %d: EATING -> THINKING\n", id);
  }

  printf(1, "Philosopher %d: finished %d cycles\n", id, CYCLES);
  exit();
}

int
main(void)
{
  int i, pid;

  for(i = 0; i < NPHIL; i++){
    forks[i] = sem_alloc(1);   // each fork starts "available"
    if(forks[i] < 0){
      printf(1, "dining: sem_alloc failed\n");
      exit();
    }
  }

  printf(1, "dining: starting %d philosophers, %d cycles each\n", NPHIL, CYCLES);

  for(i = 0; i < NPHIL; i++){
    pid = fork();
    if(pid < 0){
      printf(1, "dining: fork failed\n");
      exit();
    }
    if(pid == 0)
      philosopher(i);
  }

  for(i = 0; i < NPHIL; i++)
    wait();

  for(i = 0; i < NPHIL; i++)
    sem_free(forks[i]);

  printf(1, "dining: all philosophers finished, no deadlock\n");
  exit();
}
