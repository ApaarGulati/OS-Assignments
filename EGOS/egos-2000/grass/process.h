#pragma once

#include "egos.h"
#include "syscall.h"

enum proc_status {
    PROC_UNUSED,
    PROC_LOADING,
    PROC_READY,
    PROC_RUNNING,
    PROC_RUNNABLE,
    PROC_PENDING_SYSCALL
};

#define MLFQ_NLEVELS        5
#define MAX_NPROCESS        16
#define SAVED_REGISTER_NUM  32
#define SAVED_REGISTER_SIZE SAVED_REGISTER_NUM * 4
#define SAVED_REGISTER_ADDR (void*)(EGOS_STACK_TOP - SAVED_REGISTER_SIZE)

struct process {
    int pid;
    struct syscall syscall;
    enum proc_status status;
    uint mepc, saved_registers[SAVED_REGISTER_NUM];
    /* Student's code goes here (Preemptive Scheduler | System Call). */

    /* Add new fields for lifecycle statistics, MLFQ or process sleep. */

    ulonglong create_time;              // Process Creation time (proc_alloc)
    ulonglong first_scheduled_time;     // Time when process was first scheduled (0 when not scheduled yet)
    ulonglong termination_time;         // Process Termination time (proc_free)
    ulonglong cpu_time;                 // Total accumulated CPU time the process run for
    ulonglong last_cpu_update_time;     // Tracks start of current CPU run
    uint timer_interrupts;              // Number of timer interrupts encountered 

    int mlfq_level;                     // MLFQ Current Priority Level (Default start at 0)
    ulonglong mlfq_runtime_at_level;    // Runtime accumulated on current level

    ulonglong sleep_until;              // Time till which process is sleeping  (Default 0)

    /* Student's code ends here. */
};

ulonglong mtime_get();

int proc_alloc();
void proc_free(int);
void proc_set_ready(int);
void proc_set_running(int);
void proc_set_runnable(int);
void proc_set_pending(int);

void mlfq_reset_level();
void mlfq_update_level(struct process* p, ulonglong runtime);
void proc_sleep(int pid, uint usec);
void proc_coresinfo();

extern uint core_to_proc_idx[NCORES];
