#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_INPUT 1024   // Maximum length of input line
#define MAX_PATH 1024    // Maximum length of path for getcwd
#define MAX_ARGS 64      // Maximum number of arguments for a command
#define MAX_CMDS 10      // Maximum commands in a pipeline

int main() {
    char *line = NULL;     // User Input Line
    size_t len = 0;        // Buffer size for getline
    char cwd[MAX_PATH];    // Buffer to store current working directory

    // Shell User/Device Info
    const char *user = "User";    
    const char *device = "MyPC";


    // Ignore Ctrl+C and Ctrl+Z signals in the simpleshell (parent process) itself
    signal(SIGINT, SIG_IGN); 
    signal(SIGTSTP, SIG_IGN);

    // Main Shell Loop
    while (1) {

        // PROMPTING FOR USER INPUT
        // get current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s@%s:%s$ ", user, device, cwd);   // User@Device:path$
        } else {
            perror("getcwd");
            printf("?$ ");
        }
        fflush(stdout);

        // Reading a line from user input (stdin)
        if (getline(&line, &len, stdin) == -1) {
            break; // EOF (Ctrl+D)
        }
        
        
        
        // PARSING USER INPUT
        // remove trailing newline from user input
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        // Parsing
        char *argv[MAX_ARGS];
        int argc = 0;
        char *token = strtok(line, " \t");
        while (token && (argc < MAX_ARGS - 1)) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");          // NULL -> Continue where it left from previously
        }
        argv[argc] = NULL;     // NULL termination for the array

        if (argc == 0) continue; // empty line


        // Creating new process
        pid_t pid = fork();
        if (pid == 0) {
            // CHILD PROCESS

            // Restore default signals for child
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

             // Execute the command 
             // execvp does not return if successful
            execvp(argv[0], argv);   // (Name of program, List of arguments)
            // execvp replaces the child process with argv[0] and gives it the arguments argv

            // execvp returs if unsuccessful
            perror("execvp"); // only if exec fails
            exit(1);
        } else if (pid > 0) {
            // PARENT PROCESS
            int status; // store info about childs exit
            // Wait for the child process (the one with the given pid) to finish or stop
            waitpid(pid, &status, WUNTRACED);
            // WUNTRACED -> return not only when the child exits, but also when it is stopped.
        } else {
            perror("fork"); // in the case the fork was unsuccessfull
        }
    }

    // Free the line buffer allocated by getline (for next line)
    free(line); 
    return 0;
}