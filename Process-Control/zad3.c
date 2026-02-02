#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("\n\t-------------- PRZED --------------\n\n");
        printf("Potomek PID=%d, PPID=%d\n", getpid(), getppid());
        sleep(3);
        printf("\n\n\t-------------- PO --------------\n\n");
        printf("Potomek PID=%d, PPID=%d\n", getpid(), getppid());
    } else {
        printf("\nRodzic PID=%d \n", getpid());
        sleep(1);
        printf("\nRodzic kończy działanie.\n");
        exit(0);
    }
    return 0;
}
