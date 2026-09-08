// shm.h -- single shared page, used by Assignment 5 Q1/Q2/Q3.

#ifndef SHM_H
#define SHM_H

// Fixed user virtual address the shared page is mapped at in every
// process that calls shm_get(). Chosen well below KERNBASE (0x80000000)
// and well above any address a normal small user process's heap/stack
// will reach.
#define SHM_VA 0x70000000

void shminit(void);
int  shm_get(void);

// Used internally by vm.c's deallocuvm() so it doesn't blindly kfree
// a physical page that other processes may still have mapped.
int  shm_is_shared_page(char *kva);
void shm_release_ref(void);

#endif
