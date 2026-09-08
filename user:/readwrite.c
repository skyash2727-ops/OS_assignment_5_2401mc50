// readwrite.c -- Assignment 5, Question 3
//
// Readers-Writers problem. shared_data + read_count live on the
// shm_get() page so every reader/writer process (each is a separate
// xv6 process created with fork()) sees the same values.
//
// Synchronization: three kernel semaphores (sem.c):
//   rw_mutex   - held by whichever writer is writing, or by the
//                first reader on behalf of all current readers
//                (protects shared_data itself)
//   read_mutex - protects read_count
//   turnstile  - a "no-starve" gate: every reader AND every writer
//                must pass through it before proceeding. A reader
//                releases it immediately, but a writer holds
//                rw_mutex while turnstile is held, so once a writer
//                is waiting, at most one more already-in-flight
//                reader can sneak in ahead of it. This is the
//                classic "lightswitch + turnstile" construction
//                (Little Book of Semaphores) and is what gives
//                readers priority *among themselves* while still
//                guaranteeing a writer is served in bounded time
//                instead of starving.
//
// read_count is the count of active readers, protected by read_mutex,
// exactly as required by the assignment.

#include "types.h"
#include "stat.h"
#include "user.h"

#define NREADERS 3
#define NWRITERS 2
#define ROUNDS 4

struct rwshared {
  int shared_data;
  int read_count;
};

int rw_mutex, read_mutex, turnstile;

static void
reader(struct rwshared *s, int id)
{
  int r, val;

  for(r = 0; r < ROUNDS; r++){
    sem_wait(turnstile);
    sem_post(turnstile);

    sem_wait(read_mutex);
    s->read_count++;
    if(s->read_count == 1)
      sem_wait(rw_mutex);     // first reader locks out writers
    sem_post(read_mutex);

    // ---- read shared_data ----
    val = s->shared_data;
    printf(1, "Reader %d (pid %d): read shared_data = %d\n", id, getpid(), val);

    sem_wait(read_mutex);
    s->read_count--;
    if(s->read_count == 0)
      sem_post(rw_mutex);     // last reader lets writers back in
    sem_post(read_mutex);

    sleep(3);
  }
  exit();
}

static void
writer(struct rwshared *s, int id)
{
  int r, val;

  for(r = 0; r < ROUNDS; r++){
    sem_wait(turnstile);
    sem_wait(rw_mutex);

    // ---- exclusive write to shared_data ----
    val = ++s->shared_data;
    printf(1, "Writer %d (pid %d): wrote shared_data = %d\n", id, getpid(), val);

    sem_post(rw_mutex);
    sem_post(turnstile);

    sleep(5);
  }
  exit();
}

int
main(void)
{
  int shmva, i, pid;
  struct rwshared *s;

  shmva = shm_get();
  if(shmva < 0){
    printf(1, "readwrite: shm_get failed\n");
    exit();
  }
  s = (struct rwshared*)shmva;
  s->shared_data = 0;
  s->read_count = 0;

  rw_mutex   = sem_alloc(1);
  read_mutex = sem_alloc(1);
  turnstile  = sem_alloc(1);
  if(rw_mutex < 0 || read_mutex < 0 || turnstile < 0){
    printf(1, "readwrite: sem_alloc failed\n");
    exit();
  }

  printf(1, "readwrite: spawning %d readers and %d writers\n", NREADERS, NWRITERS);

  for(i = 0; i < NREADERS; i++){
    pid = fork();
    if(pid == 0){
      shmva = shm_get();
      s = (struct rwshared*)shmva;
      reader(s, i);
    }
  }

  for(i = 0; i < NWRITERS; i++){
    pid = fork();
    if(pid == 0){
      shmva = shm_get();
      s = (struct rwshared*)shmva;
      writer(s, i);
    }
  }

  for(i = 0; i < NREADERS + NWRITERS; i++)
    wait();

  sem_free(rw_mutex);
  sem_free(read_mutex);
  sem_free(turnstile);

  printf(1, "readwrite: all done, final shared_data = %d (expected %d)\n",
         s->shared_data, NWRITERS * ROUNDS);

  exit();
}
