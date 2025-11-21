# Bonus Assignment 
## EGOS-2000 

# EGOS-2000 OS Implementation

This is the **EGOS-2000 OS** where we have implemented CLI commands like `grep` and `wcl`. Along with that, we have also implemented an `MLFQ`.


## grep

We first read the entire file content (block by block) into a buffer and then parse the whole file line by line searching for the pattern in each line and then printing the particular lines we find the pattern in (block by block).


## wcl

We iterate through all the files, load each file content (block by block) into a buffer and then parse the whole file and count the number of newlines in the file. Then in the next iteration, we move onto the next file and repeat the process.


## MLFQ (Multi-Level Feedback Queue)

We have implemented a **Multi-level Feedback Queue** (using a **Preemptive Scheduler**) that runs processes based on their priority and only lets processes run on a particular MLFQ level for some time after which they are demoted to a lower priority level. We use **interactive process boosting** to ensure responsive shell input (resetting MLFQ on keyboard interrupts).  

We store necessary information about the process and MLFQ information in the `process.h` header file.  

We use the `proc_alloc` function to allocate the process and initialize its required information and use `proc_free` to free the process and update its necessary information.  

We use `mlfq_reset_level` function to periodically (or after keyboard interrupt for responsiveness) reset the MLFQ level of all the process and dynamically update the priority of a process in the MLFQ via the `mlfq_update_level` function based on the time it has run on a particular level.  

We use `intr_entry` to handle an interrupt and update the necessary information of a process' life and then call `proc_yield` which de-schedules the currently running process and updates its information and it then searches for a new process (with highest priority) to schedule and run on the CPU.


---

### Contributions
The work was equally divided with all aspects of the assignment done together.

**Group 44**
- Apaar Gulati - 2024095
- Bhavya Yadav - 2024152

### Github Repository
https://github.com/Krypton018/OS-Assignments/tree/main/EGOS