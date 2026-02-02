#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("\nPotomek PID=%d, PPID=%d\n", getpid(), getppid());
        sleep(2);
        printf("Potomek kończy działanie.\n\n");
        exit(0);
    } else {
        printf("\nMacierzysty PID=%d\n", getpid());
        printf("\n\t-------------- PRZED --------------\n");
        system("ps -l");
        sleep(4);
        printf("\t-------------- PO --------------\n\n");
        system("ps -l");
        wait(NULL);
    }
    return 0;
}
