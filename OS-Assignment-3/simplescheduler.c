#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>   
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h> 


// Shell variables
#define MAX_JOBS 1024
#define SHM_NAME "/ready_queue_shm"

// JOB STRUCTURE
typedef struct Job {
    pid_t pid;                // Process ID 
    char name[256];           // Executable name 
    int completed;            // 0 = running, 1 = finished
    int total_slices;         // Number of TSLICE quanta the job has RUN
    int total_wait_slices;    // Number of TSLICE quanta the job has WAITED in the ready queue
} Job;


// READY QUEUE (Circular Buffer)
typedef struct ReadyQueue {
    Job jobs[MAX_JOBS];
    int front;
    int rear;
    int count;
} ReadyQueue;

typedef struct SharedMemory {
    ReadyQueue ready_queue;     // Circular queue for scheduling
    Job all_jobs[MAX_JOBS];     // Track all jobs
    int all_jobs_count;
    int shutdown_flag;          // Signal the scheduler to exit
    sem_t mutex;                // For both ready_queue and all_jobs
} SharedMemory;

// Queue helper functions
void init_shared_mem(SharedMemory *shared_mem) {
    shared_mem->ready_queue.front = 0;
    shared_mem->ready_queue.rear = 0;
    shared_mem->ready_queue.count = 0;
    shared_mem->all_jobs_count = 0;
    // shared semaphore (pshared=1)
    if (sem_init(&shared_mem->mutex, 1, 1) == -1) {
        perror("sem_init error");
        exit(1);
    }  
}

int is_full(SharedMemory *s) {
    return s->ready_queue.count == MAX_JOBS;
}

int is_empty(SharedMemory *s) {
    return s->ready_queue.count == 0;
}

void enqueue(SharedMemory *s, Job job) {
    sem_wait(&s->mutex);
    if (is_full(s)) {
        fprintf(stderr, "Queue full! Cannot enqueue new job.\n");
        sem_post(&s->mutex);
        return;
    }
    s->ready_queue.jobs[s->ready_queue.rear] = job;
    s->ready_queue.rear = (s->ready_queue.rear + 1) % MAX_JOBS;
    s->ready_queue.count++;
    sem_post(&s->mutex);
}

int dequeue(SharedMemory *s, Job *out) {
    sem_wait(&s->mutex);
    if (is_empty(s)) {
        sem_post(&s->mutex);
        return 0;
    }
    *out = s->ready_queue.jobs[s->ready_queue.front];
    s->ready_queue.front = (s->ready_queue.front + 1) % MAX_JOBS;
    s->ready_queue.count--;
    sem_post(&s->mutex);
    return 1;
}

// GLOBAL READY QUEUE
SharedMemory *shared_mem;
int shm_fd = -1;
int NCPU;
int TSLICE;


void cleanup() {

    if (shared_mem != NULL) {
        // Unmap shared memory
        munmap(shared_mem, sizeof(SharedMemory));
        shared_mem = NULL;
    }

    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }
}

void update_all_jobs(Job finished_job) {
    sem_wait(&shared_mem->mutex);
    int found = 0;
    for (int i = 0; i < shared_mem->all_jobs_count; i++) {
        if (shared_mem->all_jobs[i].pid == finished_job.pid) {
            shared_mem->all_jobs[i] = finished_job; // Update info
            found = 1;
            break;
        }
    }
    if (!found && shared_mem->all_jobs_count < MAX_JOBS) {
        shared_mem->all_jobs[shared_mem->all_jobs_count++] = finished_job;
    }
    sem_post(&shared_mem->mutex);
}

int get_job_completion_status(pid_t pid) {
    int completed = 0;
    sem_wait(&shared_mem->mutex);
    for (int i = 0; i < shared_mem->all_jobs_count; i++) {
        if (shared_mem->all_jobs[i].pid == pid) {
            completed = shared_mem->all_jobs[i].completed;
            break;
        }
    }
    sem_post(&shared_mem->mutex);
    return completed;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE(ms)>\n", argv[0]);
        exit(1);
    }

    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

    // Currently running jobs
    Job running[NCPU];
    int running_count = 0;

    // Attach to shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open error");
        exit(1);
    }

    shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }


    // Main scheduling loop
    while (1) {
        // Check if stop condition met
        sem_wait(&shared_mem->mutex);
        int shutdown = shared_mem->shutdown_flag;
        sem_post(&shared_mem->mutex);

        if (shutdown) {
            // Stop all running jobs
            for (int i = 0; i < running_count; i++) {
                Job *job = &running[i];

                if (!get_job_completion_status(job->pid)) {
                    // Kill if still alive
                    kill(job->pid, SIGKILL);
                    job->completed = 0;  // explicitly mark incomplete
                } else {
                    job->completed = 1;  // already finished normally
                }
                update_all_jobs(*job);
            }
            running_count = 0;

            // Finish all jobs in ready queue
            Job job;
            while (dequeue(shared_mem, &job)) {
                if (!get_job_completion_status(job.pid)) {
                    kill(job.pid, SIGKILL);
                    job.completed = 0;  // explicitly mark incomplete
                } else {
                    job.completed = 1;  // already finished normally
                }
                update_all_jobs(job);
            }

            break; // exit main loop
        }

        // Stop all currently running jobs and requeue them if still alive
        for (int i = 0; i < running_count; i++) {
            Job* job = &running[i];
            int is_completed = get_job_completion_status(job->pid);
            

            if (is_completed) {
                job->completed = 1;
            } else {
                kill(job->pid, SIGSTOP);
            }
            update_all_jobs(*job);

            if (!is_completed) {
                enqueue(shared_mem, *job);
            }
        }
        running_count = 0;


        // Pick next NCPU jobs
        for (int i = 0; i < NCPU; i++) {
            Job job;
            if (!dequeue(shared_mem, &job))
                break; // no more jobs

            int is_completed = get_job_completion_status(job.pid);

            if (is_completed) {
                // Job finished while waiting, skip it, but update final status
                job.completed = 1;
                update_all_jobs(job); 
                i--; // Decrement i to try fetching another job to fill the NCPU slot
                continue;
            }

            kill(job.pid, SIGCONT);
            job.total_slices++;
            running[running_count++] = job;

            update_all_jobs(job);
        }

        // Increase waiting time for queued jobs
        sem_wait(&shared_mem->mutex);
        for (int i = 0, idx = shared_mem->ready_queue.front; i < shared_mem->ready_queue.count; i++) {
            pid_t current_pid = shared_mem->ready_queue.jobs[idx].pid;
            
            int is_completed = 0;
            for (int j = 0; j < shared_mem->all_jobs_count; j++) {
                if (shared_mem->all_jobs[j].pid == current_pid) {
                    is_completed = shared_mem->all_jobs[j].completed;
                    break;
                }
            }

            if (!is_completed) {
                shared_mem->ready_queue.jobs[idx].total_wait_slices++;
            }
            idx = (idx + 1) % MAX_JOBS;
        }
        sem_post(&shared_mem->mutex);


        usleep(TSLICE * 1000);   // milisecond -> microsecond conversion
    }

    // Cleanup 
    cleanup();
    return 0;
}