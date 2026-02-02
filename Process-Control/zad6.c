#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

int main() {
    struct sched_param param;
    pid_t pid = fork();

    if (pid == 0) {
        printf("Potomek PID=%d, PPID=%d\n", getpid(), getppid());
        param.sched_priority = 50;
        int res = sched_setscheduler(0, SCHED_RR, &param);
        if (res == -1) perror("sched_setscheduler");
        else printf("Strategia zmieniona na SCHED_RR z priorytetem 50\n");
        while (1);
    } else {
        wait(NULL);
    }
    return 0;
}
