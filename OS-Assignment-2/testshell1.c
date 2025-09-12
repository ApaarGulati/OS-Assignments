#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_CMDS 16

// Parse command into argv[], handling quotes
int parse_args(char *cmd, char **argv) {
    int argc = 0;
    char *p = cmd;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            argv[argc++] = p;
            while (*p && *p != quote) p++;
            if (*p) *p++ = '\0';
        } else {
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
    char cwd[1024];

    const char *user = "User";
    const char *device = "MyPC";

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s@%s:%s$ ", user, device, cwd);
        } else {
            perror("getcwd");
            printf("?$ ");
        }
        fflush(stdout);

        if (getline(&line, &len, stdin) == -1) break;
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        // Split by pipes
        char *cmds[MAX_CMDS];
        int num_cmds = 0;
        char *token = strtok(line, "|");
        while (token != NULL && num_cmds < MAX_CMDS) {
            while (*token == ' ') token++;  // trim leading spaces
            cmds[num_cmds++] = token;
            token = strtok(NULL, "|");
        }

        int pipefd[MAX_CMDS-1][2]; // support n-1 pipes for n commands

        // Create pipes
        for (int i = 0; i < num_cmds-1; i++) {
            if (pipe(pipefd[i]) < 0) {
                perror("pipe");
                exit(1);
            }
        }

        for (int i = 0; i < num_cmds; i++) {
            char *argv[MAX_ARGS];
            parse_args(cmds[i], argv);

            pid_t pid = fork();
            if (pid == 0) {
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

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

                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            }
        }

        // Parent closes all pipes
        for (int i = 0; i < num_cmds-1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }

        // Wait for all children
        for (int i = 0; i < num_cmds; i++) {
            wait(NULL);
        }
    }

    free(line);
    return 0;
}
