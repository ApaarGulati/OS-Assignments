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
#define MAX_ARGS 64           // Maximum number of arguments for a single command
#define MAX_NAME 256          // Maximum size of job name
#define MAX_JOBS 1024         // Maximum number of jobs allow
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
pid_t scheduler_pid = -1;

void cleanup(char* line) {
    // Free the line buffer allocated by getline
    free(line);

    if (shared_mem != NULL) {
        // Destroy semaphore in shared memory
        sem_destroy(&shared_mem->mutex);

        // Unmap shared memory
        munmap(shared_mem, sizeof(SharedMemory));
        shared_mem = NULL;
    }

    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }

    shm_unlink(SHM_NAME);
}

// Parse command into argv[] (space separated)
void parser_space(char *cmd, char **argv) {
    int argc = 0;
    char *token = strtok(cmd, " \t");
    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");     // NULL -> Continue where it left from previously
    }
    argv[argc] = NULL;   // Terminating command with NULL
}


void update_job_status_in_all_jobs(pid_t pid, int completed_status) {
    sem_wait(&shared_mem->mutex);
    for (int i = 0; i < shared_mem->all_jobs_count; i++) {
        if (shared_mem->all_jobs[i].pid == pid) {
            // Only update the status if it's being marked as complete
            // Time slices are updated by the scheduler
            shared_mem->all_jobs[i].completed = completed_status;
            break;
        }
    }
    sem_post(&shared_mem->mutex);
}


// Handling signals
void handle_signal(int sig) {
    if (sig == SIGINT) {       // Ctrl+C
        if (scheduler_pid > 0) {
            // Signal scheduler to shutdown
            sem_wait(&shared_mem->mutex);
            shared_mem->shutdown_flag = 1;  // scheduler will detect this
            sem_post(&shared_mem->mutex);

            // Wait for scheduler to finish cleanup
            waitpid(scheduler_pid, NULL, 0);
        }

        // Now print all jobs from shared memory
        if (shared_mem != NULL) {
            sem_wait(&shared_mem->mutex);
            printf("\nAll jobs:\n");
            for (int i = 0; i < shared_mem->all_jobs_count; i++) {
                Job *job = &shared_mem->all_jobs[i];
                printf("PID: %d, Name: %s, Total Completion Time (Time Slices): %d, Total Wait Time (Time Slices): %d\n",
                       job->pid, job->name, job->total_slices+job->total_wait_slices, job->total_wait_slices);
            }
            sem_post(&shared_mem->mutex);
        }
        

        cleanup(NULL);
        exit(0);
    } else if (sig == SIGCHLD) { // <-- ADDED SIGCHLD HANDLING
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            // A child process has terminated
            if (WIFEXITED(status)) {
                // exited normally
                update_job_status_in_all_jobs(pid, 1);
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                if (sig == SIGKILL || sig == SIGTERM || sig == SIGINT) {
                    // Mark as incomplete (killed by scheduler or user)
                    update_job_status_in_all_jobs(pid, 0);
                } else {
                    update_job_status_in_all_jobs(pid, 1);
                }
            }

        }
    }
}

int isValidInteger(char *s) {
    if (*s == '\0') return 0;  // empty string not allowed

    while (*s) {
        if (!isdigit(*s)) return 0; 
        s++;
    }
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE(ms)>\n", argv[0]);
        exit(1);
    }

    if (!isValidInteger(argv[1]) || !isValidInteger(argv[2])) {
        fprintf(stderr, "Error: NCPU and TSLICE must be valid positive integers.\n");
        exit(1);
    }

    int NCPU = atoi(argv[1]);
    int TSLICE = atoi(argv[2]);

    // Negative numbers will get caught by isValidInteger (These functions are just for safety)
    if (NCPU <= 0) {
        fprintf(stderr, "Error: NCPU must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }
    if (TSLICE <= 0) {
        fprintf(stderr, "Error: TSLICE must be a positive integer (in ms).\n");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;    // User Input Line
    size_t len = 0;       // Buffer size for getline

    // Handling Ctrl+C
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = handle_signal;
    if (sigaction(SIGINT, &sig, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    // Handling SIGCHLD to reap terminated jobs
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = handle_signal;
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(1);
    }


    // Open shared memory object
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(SharedMemory)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    // Shared Memory
    shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (shared_mem == MAP_FAILED) { 
        perror("mmap"); 
        exit(1); 
    }
    init_shared_mem(shared_mem);


    // Fork scheduler
    scheduler_pid = fork();
    if (scheduler_pid < 0) {
        perror("fork scheduler failed");
        exit(1);
    } else if (scheduler_pid == 0) {
        execl("./simplescheduler", "./simplescheduler", argv[1], argv[2], NULL);
        perror("execl scheduler failed");
        exit(1);
    }



    // Main Shell Loop
    while (1) {

        // PROMPTING FOR USER INPUT
        printf("SimpleShell> ");
        fflush(stdout);
        
        // Reading a line from user input (stdin)
        if (getline(&line, &len, stdin) == -1) {
            break;   // EOF Error
        }


        // PARSING USER INPUT
        line[strcspn(line, "\n")] = '\0';   // remove trailing newline from user input
        if (strlen(line) == 0) continue;    // Skipping empty line

        // Reject pipes
        if (strchr(line, '|')) {
            fprintf(stderr, "Error: Pipes are not allowed.\n");
            continue;
        }

        // Reject quotes/backslashes
        if (strchr(line, '\"') || strchr(line, '\'') || strchr(line, '\\')) {
            fprintf(stderr, "Error: Quotes and backslashes are not allowed.\n");
            continue;
        }

        // Parse the line into tokens
        char *argv[MAX_ARGS];
        parser_space(line, argv);

        // Count tokens
        int argc = 0;
        while (argv[argc] != NULL) argc++;

        // Must have exactly 2 tokens: submit <executable>
        if (argc != 2) {
            fprintf(stderr, "Error: submit command must be: submit <executable> (no args)\n");
            continue;
        }

        // First token must be "submit"
        if (strcmp(argv[0], "submit") != 0) {
            fprintf(stderr, "Error: only 'submit <executable>' is allowed.\n");
            continue;
        }



        // PROCESS EXECUTION
        // Creating new process
        pid_t pid = fork();
        if(pid < 0) {
            perror("Cannot Fork");
            exit(1);
        } else if (pid == 0) {
            // CHILD PROCESS
            
            // Execute the command 
            // execvp replaces the child process with argv[0] and gives it the arguments argv
            // execvp returns if unsuccessful
            char *child_argv[] = { argv[1], NULL };
            if (execvp(argv[1], child_argv) == -1) {
                fprintf(stderr, "SimpleShell: Command '%s' not found or not executable.\n", argv[1]);
                exit(1);
            }
        } else {
            // Enqueue on the ready queue
            Job job;
            job.pid = pid;
            strncpy(job.name, argv[1], MAX_NAME);
            job.completed = 0;
            job.total_slices = 0;
            job.total_wait_slices = 0;

            sem_wait(&shared_mem->mutex);
            if (shared_mem->all_jobs_count < MAX_JOBS) {
                shared_mem->all_jobs[shared_mem->all_jobs_count] = job;
                shared_mem->all_jobs_count++;
            } else {
                fprintf(stderr, "Error: Max jobs limit reached for history.\n");
            }
            sem_post(&shared_mem->mutex);

            enqueue(shared_mem, job);
        }
    }

    // Cleanup on exit
    cleanup(line);
    return 0;
}
