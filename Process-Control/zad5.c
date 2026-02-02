#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("\nPotomek PID=%d, PPID=%d\n", getpid(), getppid());

        int prio = nice(10);
        printf("Nowa wartość priorytetu: %d\n", prio);
        while (1);
    } else {
        wait(NULL);
    }
    return 0;
}
