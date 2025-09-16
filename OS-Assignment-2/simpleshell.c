#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Shell variables
#define MAX_INPUT 1024        // Maximum length of input line
#define MAX_ARGS 64           // Maximum number of arguments for a single command
#define MAX_CMDS 64           // Maximum number of commands separated by pipes
#define MAX_HISTORY 100       // Maximum number of commands stored in history


// Structure for storing command history and execution info
typedef struct {
    char *cmd;                     // command string
    pid_t pid;                     // process ID of the command
    struct timespec start_time;    // start time
    struct timespec end_time;      // end time
} HistoryEntry;

HistoryEntry history[MAX_HISTORY];   // Array storing history of each command
int history_count = 0;        // Number of commands in history

void cleanup(char* line) {
    // Free the line buffer allocated by getline
    free(line);

    // Free history
    for (int i = 0; i < history_count; i++) {
        free(history[i].cmd);
    }
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

// Parse line into argv[] (pipe separated)
void parser_pipe(char *cmd, char **argv, int* num_cmds) {
    char *token = strtok(cmd, "|");
    while (token != NULL && (*num_cmds) < MAX_CMDS) {
        while (*token == ' ') token++;  // trim leading spaces
        argv[(*num_cmds)++] = token;       
        token = strtok(NULL, "|");    // NULL -> Continue where it left from previously
    }

}

// Handling signals
void handle_signal(int sig) {
    if (sig == SIGINT) {       // Ctrl+C
        // Print entire history and then exit
        printf("\nExiting SimpleShell...\n");
        printf("Command Execution Summary:\n");
        for (int i = 0; i < history_count; i++) {
            // Calculating time with nanosecond precision (defined in <time.h>)
            double duration = (history[i].end_time.tv_sec - history[i].start_time.tv_sec) +
                            (history[i].end_time.tv_nsec - history[i].start_time.tv_nsec)/1e9;
            printf("PID: %d | Duration: %.6f sec | Command: %s\n", history[i].pid, duration, history[i].cmd);
        }
        
        cleanup(NULL);
        exit(0);
    } 
}

// Store info of each command in history
void store_history(const char *cmd, pid_t pid, struct timespec start, struct timespec end) {
    if (history_count < MAX_HISTORY) {
        history[history_count].cmd = strdup(cmd);
        history[history_count].pid = pid;
        history[history_count].start_time = start;
        history[history_count].end_time = end;

        history_count++;
    } else {
        // Shifting all the elements 
        free(history[0].cmd);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i-1] = history[i];
        }

        history[MAX_HISTORY-1].cmd = strdup(cmd);
        history[MAX_HISTORY-1].pid = pid;
        history[MAX_HISTORY-1].start_time = start;
        history[MAX_HISTORY-1].end_time = end;
    }
}

void update_history(pid_t pid, struct timespec start, struct timespec end) {
    history[history_count-1].pid = pid;
    history[history_count-1].start_time = start;
    history[history_count-1].end_time = end;
}

int main() {
    char *line = NULL;    // User Input Line
    size_t len = 0;       // Buffer size for getline
    char cwd[1024];       // Buffer to store current working directory (to print as a prompt)

    // Shell User/Device Info
    const char *user = "User";
    const char *device = "MyPC";

    // Handling Ctrl+C
    signal(SIGINT, handle_signal);

    // Main Shell Loop
    while (1) {

        // PROMPTING FOR USER INPUT
        // get current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s@%s:%s$ ", user, device, cwd);
        } else {
            perror("getcwd");
            printf("?$ ");
        }
        fflush(stdout);
        
        // Reading a line from user input (stdin)
        if (getline(&line, &len, stdin) == -1) {
            break;   // EOF Error
        }



        // PARSING USER INPUT
        
        line[strcspn(line, "\n")] = '\0';   // remove trailing newline from user input
        if (strlen(line) == 0) continue;    // Skipping empty line

        // Reject quotes/backslashes
        if (strchr(line, '\"') || strchr(line, '\'') || strchr(line, '\\')) {
            fprintf(stderr, "Error: Quotes and backslashes are not allowed.\n");
            continue;
        }

        // struct defined in <time.h>
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_REALTIME, &start_time);  // record time before executing command
        
        // Store in history
        store_history(line, 0, start_time, start_time);

        // Handle history command
        if (strcmp(line, "history") == 0) {
            // Printing history
            for (int i = 0; i < history_count; i++) {
                printf("%d: %s\n", i + 1, history[i].cmd);
            }

            clock_gettime(CLOCK_REALTIME, &end_time);  // record time after command finishes
            // Updating command info in history
            update_history(getpid(), start_time, end_time);

            continue;  // Skip remaining execution
        }

        // Split commands by pipes
        char *cmds[MAX_CMDS];
        int num_cmds = 0;
        parser_pipe(line, cmds, &num_cmds);



        // SETTING UP PIPES
        int pipefd[MAX_CMDS-1][2]; // support n-1 pipes for n commands
        // Create pipes
        for (int i = 0; i < num_cmds-1; i++) {
            if (pipe(pipefd[i]) < 0) {
                perror("pipe");
                exit(1);
            }
        }


        // PROCESS EXECUTION
        // Looping over all the commands one by one (creating a child process for each)
        pid_t last_pid = 0;    // Only storing the pid of the last command in a piped command chain
        for (int i = 0; i < num_cmds; i++) {
            char *argv[MAX_ARGS];             // array to store parsed command
            parser_space(cmds[i], argv);      // parsing the command (space separated)
            
            // Creating new process
            pid_t pid = fork();
            if (pid == 0) {
                // CHILD PROCESS
                // If not first command, read from previous pipe
                if (i > 0) {
                    dup2(pipefd[i-1][0], STDIN_FILENO);
                }
                // If not last command, write to next pipe
                if (i < num_cmds-1) {
                    dup2(pipefd[i][1], STDOUT_FILENO);
                }

                // Close all pipes in child
                for (int j = 0; j < num_cmds-1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                
                // Execute the command 
                // execvp replaces the child process with argv[0] and gives it the arguments argv
                // execvp returns if unsuccessful
                if (execvp(argv[0], argv) == -1) {
                    fprintf(stderr, "SimpleShell: Command '%s' not found or not executable.\n", argv[0]);
                    exit(1);
                }
            } else {
                // PARENT PROCESS
                // Storing pid of each process down the pipeline (eventually storing the last one)
                last_pid = pid;

            }

        }

        // Parent closes all pipes (They will still be present in the children)
        for (int i = 0; i < num_cmds-1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        // Wait for all children to complete
        for (int i = 0; i < num_cmds; i++) {
            wait(NULL);
        }

        // record time after command finishes
        clock_gettime(CLOCK_REALTIME, &end_time);
        // Updating final info in history
        update_history(last_pid, start_time, end_time);

    }

    // Cleanup on exit
    cleanup(line);
    return 0;
}
