#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main() {
    srand(time(NULL));
    printf("\nProces macierzysty PID=%d, PPID=%d\n\n", getpid(), getppid());

    int randomTime = rand() % 10 + 1;

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("\tPotomny 1: PID=%d, PPID=%d\n", getpid(), getppid());

            for (int j = 0; j < 2; j++) {
                pid_t pid2 = fork();
                if (pid2 == 0) {
                    printf("\t\tPotomny 2: PID=%d, PPID=%d\n", getpid(), getppid());
                    sleep(randomTime-1);
                    printf("\tProces (Potomny 2) %d kończy się po %d s.\n", getpid(), randomTime-1);
                    exit(0);
                }
            }
            sleep(randomTime);
            printf("\tProces (Potomny 1) %d kończy się po %d s.\n", getpid(), randomTime);
            while (wait(NULL) > 0);
            exit(0);
        }
    }

    while (wait(NULL) > 0);
    printf("\n");
    system("ps -l ");

    printf("\nProces macierzysty PID=%d kończy pracę.\n\n", getpid());
    return 0;
}