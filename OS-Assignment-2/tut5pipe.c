#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        // Child reads
        close(fd[1]);
        char buffer[100];
        read(fd[0], buffer, sizeof(buffer));
        printf("Child got: %s\n", buffer);
        exit(0);
    } else {
        // Parent writes
        close(fd[0]);
        write(fd[1], "Hello from parent!", 18);
        wait(NULL);
    }
    return 0;
}
