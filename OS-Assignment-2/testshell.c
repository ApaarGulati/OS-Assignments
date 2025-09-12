#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_PATH 1024
#define MAX_ARGS 64

// Function to parse command into argv[], handling quotes
int parse_args(char *cmd, char **argv) {
    int argc = 0;
    char *p = cmd;

    while (*p) {
        // Skip spaces
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '"' || *p == '\'') {
            // Quoted argument
            char quote = *p;
            p++;
            argv[argc++] = p;
            while (*p && *p != quote) p++;
            if (*p) *p++ = '\0';  // terminate argument
        } else {
            // Unquoted argument
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
        }
    }
    argv[argc] = NULL;
    return argc;
}

int main() {
    char *line = NULL;
    size_t len = 0;
    char cwd[MAX_PATH];

    const char *user = "User";
    const char *device = "MyPC";

    // Ignore Ctrl+C and Ctrl+Z in shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    while (1) {
        // Print prompt
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s@%s:%s$ ", user, device, cwd);
        } else {
            perror("getcwd");
            printf("?$ ");
        }
        fflush(stdout);

        // Read input
        if (getline(&line, &len, stdin) == -1) break;
        line[strcspn(line, "\n")] = '\0'; // remove newline
        if (strlen(line) == 0) continue;   // empty input

        // Check for pipe
        char *pipe_pos = strchr(line, '|');

        if (pipe_pos != NULL) {
            // --- PIPE HANDLING ---
            char *cmd1 = strtok(line, "|");
            char *cmd2 = strtok(NULL, "|");

            // Trim spaces
            while (*cmd1 == ' ') cmd1++;
            while (*cmd2 == ' ') cmd2++;

            // Parse cmd1 and cmd2 using parse_args
            char *argv1[MAX_ARGS];
            char *argv2[MAX_ARGS];
            int argc1 = parse_args(cmd1, argv1);
            int argc2 = parse_args(cmd2, argv2);

            // Create pipe
            int fd[2];
            if (pipe(fd) < 0) {
                perror("pipe");
                continue;
            }

            // Fork first command
            pid_t pid1 = fork();
            if (pid1 == 0) {
                // Child 1 writes to pipe
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(argv1[0], argv1);
                perror("execvp");
                exit(1);
            }

            // Fork second command
            pid_t pid2 = fork();
            if (pid2 == 0) {
                // Child 2 reads from pipe
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(argv2[0], argv2);
                perror("execvp");
                exit(1);
            }

            // Parent closes pipe and waits
            close(fd[0]);
            close(fd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);

        } else {
            // --- NORMAL COMMAND ---
            char *argv[MAX_ARGS];
            int argc = parse_args(line, argv);

            if (argc == 0) continue;

            pid_t pid = fork();
            if (pid == 0) {
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, WUNTRACED);
            } else {
                perror("fork");
            }
        }
    }

    free(line);
    return 0;
}
