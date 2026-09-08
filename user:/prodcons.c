// prodcons.c -- Assignment 5, Question 2
//
// Classic bounded-buffer producer/consumer. The buffer lives in the
// page shared between the parent (producer) and child (consumer)
// obtained via shm_get(). Synchronization uses three counting
// semaphores implemented in the kernel (sem.c) via new syscalls
// backed by xv6's sleep()/wakeup():
//   empty - counts free slots   (starts at BUFSIZE)
//   full  - counts filled slots (starts at 0)
//   mutex - binary semaphore protecting the buffer indices (starts at 1)

#include "types.h"
#include "stat.h"
#include "user.h"

#define DEFAULT_BUFSIZE 5
#define NITEMS 20

struct pcbuf {
  int buf[32];   // big enough for any bufsize we allow (<=32)
  int bufsize;
  int in;
  int out;
};

int empty_sem, full_sem, mutex_sem;

static void
producer(struct pcbuf *b)
{
  int i;

  for(i = 1; i <= NITEMS; i++){
    sem_wait(empty_sem);     // blocks here if the buffer is full
    sem_wait(mutex_sem);

    b->buf[b->in] = i;
    printf(1, "Producer: inserted %d at slot %d\n", i, b->in);
    b->in = (b->in + 1) % b->bufsize;

    sem_post(mutex_sem);
    sem_post(full_sem);

    sleep(2);  // brief pause between insertions
  }
  printf(1, "Producer: done producing %d items\n", NITEMS);
}

static void
consumer(struct pcbuf *b)
{
  int i, item;

  for(i = 1; i <= NITEMS; i++){
    sem_wait(full_sem);      // blocks here if the buffer is empty
    sem_wait(mutex_sem);

    item = b->buf[b->out];
    b->out = (b->out + 1) % b->bufsize;

    sem_post(mutex_sem);
    sem_post(empty_sem);

    printf(1, "Consumer: removed %d from slot %d\n", item, (b->out + b->bufsize - 1) % b->bufsize);

    sleep(3);  // brief pause between removals, slower than producer
               // on purpose so the buffer fills up and you can see
               // the producer block on empty_sem.
  }
  printf(1, "Consumer: done consuming %d items\n", NITEMS);
}

int
main(int argc, char *argv[])
{
  int shmva, bufsize, pid;
  struct pcbuf *b;

  bufsize = DEFAULT_BUFSIZE;
  if(argc > 1)
    bufsize = atoi(argv[1]);
  if(bufsize < 1 || bufsize > 32)
    bufsize = DEFAULT_BUFSIZE;

  shmva = shm_get();
  if(shmva < 0){
    printf(1, "prodcons: shm_get failed\n");
    exit();
  }
  b = (struct pcbuf*)shmva;
  b->bufsize = bufsize;
  b->in = 0;
  b->out = 0;

  empty_sem = sem_alloc(bufsize);
  full_sem  = sem_alloc(0);
  mutex_sem = sem_alloc(1);
  if(empty_sem < 0 || full_sem < 0 || mutex_sem < 0){
    printf(1, "prodcons: sem_alloc failed\n");
    exit();
  }

  printf(1, "prodcons: bufsize=%d, %d items\n", bufsize, NITEMS);

  pid = fork();
  if(pid < 0){
    printf(1, "prodcons: fork failed\n");
    exit();
  }

  if(pid == 0){
    // Child (consumer) needs its own mapping of the shared page.
    // The semaphore ids (empty_sem/full_sem/mutex_sem) are plain
    // integers that index a global kernel table, so the copies
    // fork() gave the child already refer to the same semaphores.
    shmva = shm_get();
    b = (struct pcbuf*)shmva;
    consumer(b);
    exit();
  } else {
    producer(b);
    wait();
    sem_free(empty_sem);
    sem_free(full_sem);
    sem_free(mutex_sem);
    printf(1, "prodcons: finished, no items lost\n");
  }

  exit();
}
