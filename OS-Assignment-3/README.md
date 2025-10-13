# OS Assignment - 3
## Simple Scheduler

This is a simple scheduler that executes simple executables with the **submit** command (according to condition **1-d** of the PDF, i.e., it should not have any blocking call, it should not have any command line argument, and it cannot have pipes).  

The simple scheduler has been connected with the **Simple Shell (Assignment-2)**.  

The 'virtual computer' has **N CPUs** and **T time slice**. The scheduler runs **N processes** (if available) at a time for the time slice, then puts them back into the ready queue in **round robin** fashion, and then runs the next N processes for the time slice.

All the information about the processes is stored in the **shared memory** that is shared between the shell and the scheduler. Proper **semaphores and locks** have been implemented to avoid race conditions and deadlocks in the critical section. The shell keeps adding new processes to the shared memory ready queue, and the scheduler just moves them around in a round robin fashion and removes them (if finished) from the ready queue.

We have a **shutdown flag** in the shared memory that the shell sets on termination, and the scheduler checks it for each iteration it runs. If it is set, it kills all the running jobs and terminates the scheduler itself too.

We have to add a **"dummy_main.h"** header file in all the executables to make them compatible with our scheduler. The `dummy_main` is just to replace the actual `main` function of the executable so that the executable doesn't directly run when `exec` is called, but is instead sent to the ready queue where the scheduler will decide when it runs.


---

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignments/tree/main/OS-Assignment-3