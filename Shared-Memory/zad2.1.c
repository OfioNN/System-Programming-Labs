#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define SHM_NAME  "/pc_shm_42"
#define SEM_EMPTY "/sem_empty_42"
#define SEM_FULL  "/sem_full_42"
#define SEM_MUTEX "/sem_mutex_42"

#define BUF_SIZE 10

typedef struct {
    int godzina;
    int pid;
    float temperatura;
    int wilgotnosc;
} dane_t;

typedef struct {
    dane_t buf[BUF_SIZE];
    int in;
    int out;
    int count;
    int end;
} shm_t;

static void producent(shm_t *shm, sem_t *empty, sem_t *full, sem_t *mutex, int fd) {
    srand((unsigned)getpid());

    while (1) {
        if (shm->end) break;

        dane_t d;
        d.godzina = rand() % 2400;
        d.pid = getpid();
        d.temperatura = (rand() % 400) / 10.0f;
        d.wilgotnosc = rand() % 101;

        sem_wait(empty);
        sem_wait(mutex);

        if (shm->end) {
            sem_post(mutex);
            sem_post(empty);
            break;
        }

        shm->buf[shm->in] = d;
        shm->in = (shm->in + 1) % BUF_SIZE;
        shm->count++;

        printf("Producent_%d: dane wpisane: Time=%04d Temp=%.1f Hum=%d%%\n",
               getpid(), d.godzina, d.temperatura, d.wilgotnosc);

        dprintf(fd,
                "Producent_%d umieścił produkt, ilość elementów w buforze: %d\n",
                getpid(), shm->count);

        sem_post(mutex);
        sem_post(full);

        usleep((rand() % 1000) * 1000);
    }

    _exit(0);
}

static void konsument(shm_t *shm, sem_t *empty, sem_t *full, sem_t *mutex, int fd) {
    srand((unsigned)getpid());

    while (1) {
        sem_wait(full);
        sem_wait(mutex);

        if (shm->end && shm->count == 0) {
            sem_post(mutex);
            break;
        }

        if (shm->count <= 0) {
            sem_post(mutex);
            sem_post(full);
            usleep(1000);
            continue;
        }

        dane_t d = shm->buf[shm->out];
        shm->out = (shm->out + 1) % BUF_SIZE;
        shm->count--;

        printf("Konsument_%d: dane pobrane: Time=%04d Temp=%.1f Hum=%d%%\n",
               getpid(), d.godzina, d.temperatura, d.wilgotnosc);

        dprintf(fd,
                "Konsument_%d pobrał produkt, ilość elementów w buforze: %d\n",
                getpid(), shm->count);

        sem_post(mutex);
        sem_post(empty);

        usleep((rand() % 1000) * 1000);
    }

    _exit(0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s producenci konsumenci\n", argv[0]);
        return 1;
    }

    int P = atoi(argv[1]);
    int K = atoi(argv[2]);
    if (P < 0 || K < 0) {
        fprintf(stderr, "Błędne wartości P/K\n");
        return 1;
    }

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); return 1; }

    if (ftruncate(shm_fd, sizeof(shm_t)) == -1) { perror("ftruncate"); return 1; }

    shm_t *shm = mmap(NULL, sizeof(shm_t),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) { perror("mmap"); return 1; }

    memset(shm, 0, sizeof(shm_t));

    sem_t *empty = sem_open(SEM_EMPTY, O_CREAT, 0666, BUF_SIZE);
    sem_t *full  = sem_open(SEM_FULL,  O_CREAT, 0666, 0);
    sem_t *mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    if (empty == SEM_FAILED || full == SEM_FAILED || mutex == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    int fd = open("zad4_2_log.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd == -1) { perror("open log"); return 1; }

    for (int i = 0; i < P; i++) {
        pid_t pid = fork();
        if (pid == 0) producent(shm, empty, full, mutex, fd);
        if (pid < 0) { perror("fork producent"); return 1; }
    }

    for (int i = 0; i < K; i++) {
        pid_t pid = fork();
        if (pid == 0) konsument(shm, empty, full, mutex, fd);
        if (pid < 0) { perror("fork konsument"); return 1; }
    }

    printf("Naciśnij 'q' + Enter aby zakończyć...\n");
    char c;
    while (scanf(" %c", &c) == 1 && c != 'q') {}

    sem_wait(mutex);
    shm->end = 1;
    sem_post(mutex);

    for (int i = 0; i < K; i++) sem_post(full);

    while (wait(NULL) > 0) {}

    close(fd);
    munmap(shm, sizeof(shm_t));

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_MUTEX);

    return 0;
}
