#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define NPHIL 5

// Sygnały (realtime, żeby móc używać sigqueue)
#define SIG_REQUEST   (SIGRTMIN)     // filozof -> kelner: prośba o widelce (głodny)
#define SIG_RELEASE   (SIGRTMIN + 1) // filozof -> kelner: zwolnienie widelców
#define SIG_PERMISSION (SIGRTMIN + 2) // kelner -> filozof: zgoda na jedzenie

// Stany filozofów
typedef enum { THINKING = 0, HUNGRY = 1, EATING = 2 } state_t;

// KOD FILOZOFA (CHILD)

void philosopher_process(int id, pid_t waiter_pid) {
    // numery widelców
    int left = id;
    int right = (id + 1) % NPHIL;

    srand(time(NULL) ^ getpid());

    // Maski do sigwait
    sigset_t perm_mask;
    sigemptyset(&perm_mask);
    sigaddset(&perm_mask, SIG_PERMISSION);

    for (int cycle = 0; cycle < 5; ++cycle) {
        int think_time = (rand() % 5) + 1;
        int eat_time   = (rand() % 5) + 1;

        printf("filozof_%d: myśli (%ds)\n", id, think_time);
        fflush(stdout);
        sleep(think_time);

        // Staje się głodny
        printf("filozof_%d: głodny\n", id);
        fflush(stdout);

        // Wysyła prośbę do kelnera o widelce
        union sigval sv;
        sv.sival_int = id;
        if (sigqueue(waiter_pid, SIG_REQUEST, sv) == -1) {
            perror("sigqueue SIG_REQUEST");
        }

        // Czeka na sygnał zgody od kelnera
        int sig;
        sigwait(&perm_mask, &sig); // SIG_PERMISSION

        // Otrzymano zgodę
        printf("filozof_%d: zajmuje widelce: %d i %d\n", id, left, right);
        printf("filozof_%d: je (%ds)\n", id, eat_time);
        fflush(stdout);
        sleep(eat_time);

        // Zwalnia widelce i informuje kelnera
        printf("filozof_%d: zwalnia widelce: %d i %d\n", id, left, right);
        fflush(stdout);
        sv.sival_int = id;
        if (sigqueue(waiter_pid, SIG_RELEASE, sv) == -1) {
            perror("sigqueue SIG_RELEASE");
        }
    }

    printf("filozof_%d: kończy działanie.\n", id);
    fflush(stdout);
    _exit(0);
}

// KOD KELNERA (PARENT)

int main(void) {
    sigset_t block_all;
    sigemptyset(&block_all);
    sigaddset(&block_all, SIG_REQUEST);
    sigaddset(&block_all, SIG_RELEASE);
    sigaddset(&block_all, SIG_PERMISSION);
    sigaddset(&block_all, SIGCHLD);

    // Blokujemy sygnały w całym procesie (sigwaitinfo)
    if (sigprocmask(SIG_BLOCK, &block_all, NULL) == -1) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    pid_t waiter_pid = getpid();
    pid_t philosophers[NPHIL];

    // Stan widelców i filozofów
    int fork_free[NPHIL];
    state_t state[NPHIL];

    // Początkowo wszyscy "myślą"i wszzystkie widelce wolne
    for (int i = 0; i < NPHIL; ++i) {
        fork_free[i] = 1;
        state[i] = THINKING;
    }

    // Tworzenie 5 procesów filozofów
    for (int i = 0; i < NPHIL; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Wywołanie procesu filozofa
            philosopher_process(i, waiter_pid);
        } else {
            philosophers[i] = pid;
        }
    }

    // Maska dla sigwaitinfo – (REQUEST i RELEASE)
    sigset_t waiter_mask;
    sigemptyset(&waiter_mask);
    sigaddset(&waiter_mask, SIG_REQUEST);
    sigaddset(&waiter_mask, SIG_RELEASE);
    sigaddset(&waiter_mask, SIGCHLD); 

    int running_philos = NPHIL;

    while (running_philos > 0) {
        siginfo_t info;
        int sig = sigwaitinfo(&waiter_mask, &info);
        if (sig == -1) {
            perror("sigwaitinfo");
            continue;
        }

        if (sig == SIGCHLD) {
            int status;
            pid_t res;
            while ((res = waitpid(-1, &status, WNOHANG)) > 0) {
                running_philos--;
            }
            continue; // wracamy do czekania na kolejne sygnały
        }

        int id = info.si_value.sival_int;
        int left = id;
        int right = (id + 1) % NPHIL;

        if (sig == SIG_REQUEST) {
            state[id] = HUNGRY;
        } else if (sig == SIG_RELEASE) {
            fork_free[left] = 1;
            fork_free[right] = 1;
            state[id] = THINKING;
        }

        for (int i = 0; i < NPHIL; ++i) {
            if (state[i] == HUNGRY) {
                int l = i;
                int r = (i + 1) % NPHIL;
                if (fork_free[l] && fork_free[r]) {
                    fork_free[l] = 0;
                    fork_free[r] = 0;
                    state[i] = EATING;

                    union sigval sv;
                    sv.sival_int = i;
                    if (sigqueue(philosophers[i], SIG_PERMISSION, sv) == -1) {
                        perror("sigqueue SIG_PERMISSION");
                    }
                }
            }
        }
    }

    printf("Kelner: wszyscy filozofowie zakończyli działanie.\n");
    return EXIT_SUCCESS;
}
