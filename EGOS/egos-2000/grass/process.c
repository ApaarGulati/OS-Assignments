/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: helper functions for process management
 */

#include "process.h"


#define MLFQ_RESET_PERIOD     10000000         /* 10 seconds */
#define MLFQ_LEVEL_RUNTIME(x) (x + 1) * 100000 /* e.g., 100ms for level 0 */
extern struct process proc_set[MAX_NPROCESS + 1];

static void proc_set_status(int pid, enum proc_status status) {
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].pid == pid) proc_set[i].status = status;
}

void proc_set_ready(int pid) { proc_set_status(pid, PROC_READY); }
void proc_set_running(int pid) { proc_set_status(pid, PROC_RUNNING); }
void proc_set_runnable(int pid) { proc_set_status(pid, PROC_RUNNABLE); }
void proc_set_pending(int pid) { proc_set_status(pid, PROC_PENDING_SYSCALL); }

int proc_alloc() {
    static uint curr_pid = 0;
    for (uint i = 1; i <= MAX_NPROCESS; i++)
        if (proc_set[i].status == PROC_UNUSED) {
            proc_set[i].pid    = ++curr_pid;
            proc_set[i].status = PROC_LOADING;
            /* Student's code goes here (Preemptive Scheduler | System Call). */

            /* Initialization of lifecycle statistics, MLFQ or process sleep. */
            proc_set[i].create_time            = mtime_get();
            proc_set[i].first_scheduled_time   = 0;
            proc_set[i].termination_time       = 0;
            proc_set[i].cpu_time               = 0;
            proc_set[i].last_cpu_update_time   = 0;
            proc_set[i].timer_interrupts       = 0;

            proc_set[i].mlfq_level             = 0;
            proc_set[i].mlfq_runtime_at_level  = 0;

            proc_set[i].sleep_until            = 0;

            /* Student's code ends here. */
            return curr_pid;
        }

    FATAL("proc_alloc: reach the limit of %d processes", MAX_NPROCESS);
}

void print_process_stats(struct process* p) {
    ulonglong turnaround = 0, response = 0;

    // Calculating turnaround time
    if (p->termination_time >= p->create_time)
        turnaround = p->termination_time - p->create_time;

    // Calculating response time
    if (p->first_scheduled_time != 0 && p->first_scheduled_time >= p->create_time)
        response = p->first_scheduled_time - p->create_time;

    // Convert from microseconds to milliseconds
    turnaround /= 1000;
    response   /= 1000;
    p->cpu_time /= 1000;

    // Splitting 64-bit numbers into 2x32-bit for printing
    int cpu_time_low  = (int)(p->cpu_time & 0xFFFFFFFF);
    int cpu_time_high = (int)(p->cpu_time >> 32);

    int turnaround_low  = (int)(turnaround & 0xFFFFFFFF);
    int turnaround_high = (int)(turnaround >> 32);

    int response_low  = (int)(response & 0xFFFFFFFF);
    int response_high = (int)(response >> 32);

    printf("\n---------------------------------------------------------\n");
    printf("Process Finished:\n");
    printf("---------------------------------------------------------\n");
    printf("PID             : %d\n", p->pid);

    printf("Turnaround Time : ");
    if (turnaround_high > 0) printf("%d", turnaround_high);
    printf("%d ms\n", turnaround_low);

    printf("Response Time   : ");
    if (response_high > 0) printf("%d", response_high);
    printf("%d ms\n", response_low);

    printf("CPU Time        : ");
    if (cpu_time_high > 0) printf("%d", cpu_time_high);
    printf("%d ms\n", cpu_time_low);

    printf("Timer Interrupts: %d\n", p->timer_interrupts);
    printf("Final MLFQ Level: %d\n", p->mlfq_level);
    printf("---------------------------------------------------------\n\n");
}

void proc_free(int pid) {
    /* Student's code goes here (Preemptive Scheduler). */

    /* Print the lifecycle statistics of the terminated process or processes. */
    if (pid != GPID_ALL) {
        // Find the process entry using pid to print stats before freeing //
        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].pid == pid && proc_set[i].status != PROC_UNUSED) {
                // Record termination time 
                proc_set[i].termination_time = mtime_get();
                // Print statistics 
                print_process_stats(&proc_set[i]);
                break;
            }
        }

        earth->mmu_free(pid);
        proc_set_status(pid, PROC_UNUSED);
    } else {
        /* Free all user processes. */
        for (uint i = 0; i < MAX_NPROCESS; i++)
            if (proc_set[i].pid >= GPID_USER_START && proc_set[i].status != PROC_UNUSED) {
                // Print stats for each user process before freeing 
                proc_set[i].termination_time = mtime_get();
                // Print statistics 
                print_process_stats(&proc_set[i]);

                earth->mmu_free(proc_set[i].pid);
                proc_set[i].status = PROC_UNUSED;
            }
    }
    /* Student's code ends here. */
}

void mlfq_update_level(struct process* p, ulonglong runtime) {
    /* Student's code goes here (Preemptive Scheduler). */

    /* Update the MLFQ-related fields in struct process* p after this
     * process has run on the CPU for another runtime microseconds. */
    
    // Add runtime to level runtime tracker 
    p->mlfq_runtime_at_level += runtime;

    // Demote through levels if runtime on this level exceeds threshold
    while (p->mlfq_level < (MLFQ_NLEVELS - 1)) {
        ulonglong threshold = MLFQ_LEVEL_RUNTIME(p->mlfq_level);
        if (p->mlfq_runtime_at_level > threshold) {
            // subtract threshold and move down one level
            p->mlfq_runtime_at_level -= threshold;
            p->mlfq_level++;
        } else {
            break;
        }
    }

    /* Student's code ends here. */
}

void mlfq_reset_level() {
    /* Student's code goes here (Preemptive Scheduler). */
    if (!earth->tty_input_empty()) {
        /* Reset the level of GPID_SHELL if there is pending keyboard input. */
        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].pid == GPID_SHELL && proc_set[i].status != PROC_UNUSED) {
                proc_set[i].mlfq_level = 0;
                proc_set[i].mlfq_runtime_at_level = 0;
                break;
            }
        }
    }

    static ulonglong MLFQ_last_reset_time = 0;
    /* Reset the level of all processes every MLFQ_RESET_PERIOD microseconds. */
    ulonglong now = mtime_get();
    if (MLFQ_last_reset_time == 0) MLFQ_last_reset_time = now;

    if (now - MLFQ_last_reset_time >= MLFQ_RESET_PERIOD) {
        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].status != PROC_UNUSED) {
                proc_set[i].mlfq_level = 0;
                proc_set[i].mlfq_runtime_at_level = 0;
            }
        }
        MLFQ_last_reset_time = now;
    }

    /* Student's code ends here. */
}

void proc_sleep(int pid, uint usec) {
    /* Student's code goes here (System Call & Protection). */

    /* Update the sleep-related fields in the struct process for process pid. */

    /* Student's code ends here. */
}

void proc_coresinfo() {
    /* Student's code goes here (Multicore & Locks). */

    /* Print out the pid of the process running on each CPU core. */

    /* Student's code ends here. */
}
