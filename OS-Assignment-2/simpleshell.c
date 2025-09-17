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

// Array to store original user input lines for `history` command
char *original_lines[MAX_HISTORY];
int orig_count = 0;

void cleanup(char* line) {
    // Free the line buffer allocated by getline
    free(line);

    // Free history
    for (int i = 0; i < history_count; i++) {
        free(history[i].cmd);
    }

    for (int i = 0; i < orig_count; i++) {
        free(original_lines[i]);
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



int main() {
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

        // Reject quotes/backslashes
        if (strchr(line, '\"') || strchr(line, '\'') || strchr(line, '\\')) {
            fprintf(stderr, "Error: Quotes and backslashes are not allowed.\n");
            continue;
        }

        // struct defined in <time.h>
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_REALTIME, &start_time);  // record time before executing command

        
        // Store the original line for history command
        if (orig_count < MAX_HISTORY) {
            original_lines[orig_count++] = strdup(line);
        } else {
            free(original_lines[0]);
            for (int i = 1; i < MAX_HISTORY; i++) {
                original_lines[i-1] = original_lines[i];
            }
            original_lines[MAX_HISTORY-1] = strdup(line);
        }

        // Split commands by pipes
        char *cmds[MAX_CMDS];
        int num_cmds = 0;
        parser_pipe(line, cmds, &num_cmds);

        // Store each piped command separately for termination summary
        for (int i = 0; i < num_cmds; i++) {
            if (history_count < MAX_HISTORY) {
                history[history_count].cmd = strdup(cmds[i]);
                history[history_count].pid = 0;
                history[history_count].start_time = start_time;
                history[history_count].end_time = start_time;
                history_count++;
            } else {
                // Shift history if full
                free(history[0].cmd);
                for (int j = 1; j < MAX_HISTORY; j++) {
                    history[j-1] = history[j];
                }
                history[MAX_HISTORY-1].cmd = strdup(cmds[i]);
                history[MAX_HISTORY-1].pid = 0;
                history[MAX_HISTORY-1].start_time = start_time;
                history[MAX_HISTORY-1].end_time = start_time;
            }
        }

        // Handle history command
        if (strcmp(line, "history") == 0) {
            // Printing history
            for (int i = 0; i < orig_count; i++) {
                printf("%d: %s\n", i + 1, original_lines[i]);
            }

            clock_gettime(CLOCK_REALTIME, &end_time);  // record time after command finishes
            // Updating command info in history
            history[history_count-1].pid = getpid();
            history[history_count-1].end_time = end_time;

            continue;  // Skip remaining execution
        }



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
                // Storing child info in history
                int hist_idx = history_count - num_cmds + i;
                history[hist_idx].pid = pid;
                history[hist_idx].start_time = start_time;

            }
        }

        // Parent closes all pipes (They will still be present in the children)
        for (int i = 0; i < num_cmds-1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        // Wait for all children to complete
        for (int i = 0; i < num_cmds; i++) {
            int status;
            wait(&status);
            clock_gettime(CLOCK_REALTIME, &end_time);
            int hist_idx = history_count - num_cmds + i;
            history[hist_idx].end_time = end_time;
        }

    }

    // Cleanup on exit
    cleanup(line);
    return 0;
}
