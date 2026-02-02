#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

static volatile sig_atomic_t primes_received = 0;

int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return 0;
    }
    return 1;
}

static void prime_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)sig;
    (void)ucontext;

    int prime = info->si_value.sival_int;
    primes_received++;
    printf("Otrzymano liczbę pierwszą: %d\n", prime);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Użycie: %s Zd Zg n_procesów\n", argv[0]);
        return EXIT_FAILURE;
    }

    int Zd = atoi(argv[1]);
    int Zg = atoi(argv[2]);
    int n  = atoi(argv[3]);

    if (Zd > Zg || n <= 0) {
        fprintf(stderr, "Błędne argumenty.\n");
        return EXIT_FAILURE;
    }

    // Ustawienie obsługi sygnału
    struct sigaction sa;
    sa.sa_sigaction = prime_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    const int SIG_PRIME = SIGRTMIN;

    if (sigaction(SIG_PRIME, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // Podział zakresu na n procesów
    int total_numbers = Zg - Zd + 1;
    int base = total_numbers / n;    // minimalna długość kawałka
    int rem  = total_numbers % n;    // liczba o 1 dłuższych

    for (int i = 0; i < n; ++i) {
        int extra = (i < rem) ? 1 : 0;
        int len   = base + extra;

        int offset = i * base + (i < rem ? i : rem);
        int start  = Zd + offset;
        int end    = start + len - 1;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return EXIT_FAILURE;
        }


        // proces potomny
        if (pid == 0) {
            int count = 0;
            union sigval sv;

            for (int x = start; x <= end; ++x) {
                if (is_prime(x)) {
                    count++;
                    sv.sival_int = x;
                    if (sigqueue(getppid(), SIG_PRIME, sv) == -1) {
                        perror("sigqueue");
                    }
                }
            }

            _exit(count & 0xFF);
        }
    }

    // zbieranie wyników od potomków
    int total_primes_from_children = 0;

    for (int i = 0; i < n; ++i) {
        int status;
        pid_t pid;

        // wait może zostać przerwany sygnałem -> powtarzamy
        do {
            pid = wait(&status);
        } while (pid == -1 && errno == EINTR);

        if (pid == -1) {
            perror("wait");
            break;
        }

        if (WIFEXITED(status)) {
            total_primes_from_children += WEXITSTATUS(status);
        }
    }

    printf("\nPodsumowanie:\n");
    printf("  Liczba odebranych sygnałów: %d\n", primes_received);
    printf("  Suma wartości zwróconych przez potomne: %d\n",
           total_primes_from_children);

    if (primes_received == total_primes_from_children) {
        printf("=> Wszystkie liczby pierwsze zostały poprawnie odebrane.\n");
    } else {
        printf("=> ROZBIEŻNOŚĆ! Odebrano %d sygnałów, a potomne zgłosiły %d.\n",
               primes_received, total_primes_from_children);
    }

    return EXIT_SUCCESS;
}