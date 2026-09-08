// shm.c -- Assignment 5: a minimal shared-memory facility.
//
// xv6 gives every process its own private page table; there is no
// mmap/shmget. For this assignment we only need ONE page shared
// between a parent and (typically) a handful of its children, so we
// keep a single global physical page. shm_get() maps that SAME
// physical page into the calling process's address space at a fixed
// virtual address (SHM_VA). Any two processes that both call
// shm_get() therefore see the same memory there.
//
// Because xv6's page-freeing code (deallocuvm/freevm in vm.c) always
// kfree()s a present user page when a process exits, and our shared
// page is mapped into MORE THAN ONE process's page table, we cannot
// let the normal path free it -- otherwise the first process to exit
// would free memory the other process is still using, corrupting the
// free list. So vm.c calls shm_is_shared_page()/shm_release_ref()
// instead of kfree() for this one page, and we keep a manual
// reference count: the underlying physical page is only actually
// freed once every process that mapped it has exited.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "shm.h"

static char *shmpage = 0;   // kernel virtual address of the shared page
static int shmrefcnt = 0;   // number of processes currently mapping it
static struct spinlock shmlock;

void
shminit(void)
{
  initlock(&shmlock, "shmlock");
  shmpage = 0;
  shmrefcnt = 0;
}

// Map the global shared page into the calling process at SHM_VA,
// allocating the physical page the first time it is needed.
// Returns SHM_VA on success, -1 on failure.
int
shm_get(void)
{
  struct proc *curproc = myproc();
  pte_t *pte;

  acquire(&shmlock);

  if(shmpage == 0){
    if((shmpage = kalloc()) == 0){
      release(&shmlock);
      return -1;
    }
    memset(shmpage, 0, PGSIZE);
  }

  // If this process already has it mapped (e.g. it called shm_get()
  // more than once), just hand back the address without remapping --
  // mappages() would panic on a remap.
  pte = walkpgdir(curproc->pgdir, (void*)SHM_VA, 0);
  if(pte != 0 && (*pte & PTE_P)){
    release(&shmlock);
    return SHM_VA;
  }

  if(mappages(curproc->pgdir, (void*)SHM_VA, PGSIZE, V2P(shmpage), PTE_W|PTE_U) < 0){
    release(&shmlock);
    return -1;
  }
  shmrefcnt++;

  release(&shmlock);
  return SHM_VA;
}

// Is kva the (kernel virtual address of the) shared page?
int
shm_is_shared_page(char *kva)
{
  return shmpage != 0 && kva == shmpage;
}

// A process that had the shared page mapped is tearing down its
// address space; drop one reference and free the page once nobody
// maps it any more.
void
shm_release_ref(void)
{
  acquire(&shmlock);
  if(shmrefcnt > 0)
    shmrefcnt--;
  if(shmrefcnt == 0 && shmpage != 0){
    kfree(shmpage);
    shmpage = 0;
  }
  release(&shmlock);
}
