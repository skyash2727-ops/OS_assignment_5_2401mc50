# OS_assignment_5_2401mc50
Process Synchronization using xv6

Question 1: Peterson's Algorithm
Design
Peterson's Algorithm lets two processes share a critical section using only software variables (flag[] and turn), without any OS locks.  

Setup: The parent calls shm_get() to create the shared page and sets flag[0] = 0, flag[1] = 0, turn = 0, and shared_counter = 0. The child maps the same page using shm_get() after fork().  

Entry: Process i sets flag[i] = 1 and gives the turn to the other process (turn = other). It then waits in a while loop if the other process wants to enter and it is their turn.  

Critical Section: The process increments shared_counter and prints its status.  

Exit: Process i sets flag[i] = 0 so the other process can enter.  

Output Verification
Both parent and child run 10 loops each. The final counter value is exactly 20. No two processes enter the critical section at the same time.  

Question 2: Producer-Consumer (Bounded Buffer)
Design
We use a fixed array of size 5 (struct pcbuf) on our shared page and manage it with three semaphores:  

empty (starts at 5): Counts how many empty spots are left.  
full (starts at 0): Counts how many items are in the buffer.  
mutex (starts at 1): Ensures only one process modifies the buffer at a time.  

The parent creates the semaphores before calling fork(). The child gets copies of the semaphore ID numbers automatically because fork() duplicates user memory.  

Output Verification
The producer calls sem_wait(empty) and pauses when the buffer reaches 5 items.  
The consumer calls sem_wait(full) and pauses when the buffer has 0 items.  
No items are lost, duplicated, or read out of order.  

Question 3: Readers-Writers Problem
Design

We implemented the First Readers-Writers problem (readers get priority) with a turnstile lock so writers do not starve:  
read_mutex: Protects the read_count variable.
rw_mutex: Locked by the first reader and unlocked by the last reader. Writers lock this to get exclusive access.  
turnstile: A gate that both readers and writers must pass through. Readers pass through immediately, but writers hold it until they finish writing. This stops new readers from crowding out waiting writers.

Output Verification
Spawning 3 readers and 2 writers shows that multiple readers can read at the same time, but writers always get exclusive access (no readers or other writers active during a write).  

Question 4: Dining Philosophers Problem
Design
We represent 5 forks using 5 binary semaphores (initialized to 1). Each philosopher is a separate process created with fork(). We do not need shm_get() here because each philosopher only needs its two fork IDs.  

To prevent deadlock, we use an asymmetric lock strategy:  

Even philosophers pick up their left fork first, then their right.  
Odd philosophers pick up their right fork first, then their left.  

This breaks the circular wait condition—all 5 philosophers can never grab one fork at the same time and freeze.  

Output Verification
Each philosopher cycles through THINKING -> HUNGRY -> EATING -> THINKING for 5 rounds. All philosophers finish without any deadlock or system freezing.
