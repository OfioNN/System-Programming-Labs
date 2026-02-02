#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("Potomek PID=%d, PPID=%d\n", getpid(), getppid());
        sleep(1);
        execl("./time", NULL);
        perror("exec error");
    } else {
        printf("\nRodzic PID=%d\n", getpid());
        printf("\n\t-------------- PRZED --------------\n\n");
        system("ps -l");
        sleep(2);
        printf("\t-------------- PO --------------\n\n");
        system("ps -l");
    }
    return 0;
}
