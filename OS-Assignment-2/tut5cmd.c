#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

    int fd[2];
    pipe(fd);

    printf("in parent process\n\n");

    pid_t child1 = fork();

    if (child1 < 0) {
        perror("fork child1 failed");
        exit(1);
    } else if (child1 == 0) {
        printf("in first child\n\n");

        pid_t child2 = fork();
        if (child2 < 0) {
            perror("fork child2 failed");
            exit(1);
        } else if (child2 == 0) {
            printf("in second child\n\n");
            printf("--------START OF EXEC BLOCK--------\n\n");
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);

            char *args[] = {"cat", "clickit", NULL};
            execvp(args[0], args);
        }

        wait(NULL);

        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);

        // char *args[] = {"grep", "why", NULL};
        char *args[] = {"grep", "-c", "why", NULL};
        execvp(args[0], args);
    }

    close(fd[0]);
    close(fd[1]);

    wait(NULL);

    printf("\n---------END OF EXEC BLOCK---------\n\n");

    printf("back in parent process\n\n");
    return 0;
}
