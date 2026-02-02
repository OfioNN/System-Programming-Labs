#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#define SHM_NAME "/pc_shm_43"
#define BUF_SIZE 10

typedef struct {
    int godzina;
    int pid;
    float temperatura;
    int wilgotnosc;
} dane_t;

typedef struct {
    dane_t buf[BUF_SIZE];
    int in, out, count;
    int end;
    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} shm_t;

void producent(shm_t *shm, int fd) {
    srand(getpid());

    while (1) {
        pthread_mutex_lock(&shm->mtx);
        while (shm->count == BUF_SIZE && !shm->end)
            pthread_cond_wait(&shm->not_full, &shm->mtx);

        if (shm->end) {
            pthread_mutex_unlock(&shm->mtx);
            break;
        }

        dane_t d;
        d.godzina = rand() % 2400;
        d.pid = getpid();
        d.temperatura = (rand() % 400) / 10.0;
        d.wilgotnosc = rand() % 100;

        shm->buf[shm->in] = d;
        shm->in = (shm->in + 1) % BUF_SIZE;
        shm->count++;

        printf("Producent_%d: Time=%04d Temp=%.1f Hum=%d%%\n",
               getpid(), d.godzina, d.temperatura, d.wilgotnosc);

        dprintf(fd,
                "Producent_%d umieścił produkt, ilość elementów: %d\n",
                getpid(), shm->count);

        pthread_cond_signal(&shm->not_empty);
        pthread_mutex_unlock(&shm->mtx);

        usleep((rand() % 1000) * 1000);
    }
    exit(0);
}

void konsument(shm_t *shm, int fd) {
    srand(getpid());

    while (1) {
        pthread_mutex_lock(&shm->mtx);
        while (shm->count == 0 && !shm->end)
            pthread_cond_wait(&shm->not_empty, &shm->mtx);

        if (shm->count == 0 && shm->end) {
            pthread_mutex_unlock(&shm->mtx);
            break;
        }

        dane_t d = shm->buf[shm->out];
        shm->out = (shm->out + 1) % BUF_SIZE;
        shm->count--;

        printf("Konsument_%d: Time=%04d Temp=%.1f Hum=%d%%\n",
               getpid(), d.godzina, d.temperatura, d.wilgotnosc);

        dprintf(fd,
                "Konsument_%d pobrał produkt, ilość elementów: %d\n",
                getpid(), shm->count);

        pthread_cond_signal(&shm->not_full);
        pthread_mutex_unlock(&shm->mtx);

        usleep((rand() % 1000) * 1000);
    }
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Użycie: %s producenci konsumenci\n", argv[0]);
        return 1;
    }

    int P = atoi(argv[1]);
    int K = atoi(argv[2]);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shm_t));
    shm_t *shm = mmap(NULL, sizeof(shm_t),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memset(shm, 0, sizeof(shm_t));

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shm->mtx, &mattr);
    pthread_cond_init(&shm->not_empty, &cattr);
    pthread_cond_init(&shm->not_full, &cattr);

    int fd = open("zad4_3_log.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);

    for (int i = 0; i < P; i++)
        if (!fork()) producent(shm, fd);

    for (int i = 0; i < K; i++)
        if (!fork()) konsument(shm, fd);

    printf("Naciśnij 'q' + Enter aby zakończyć...\n");
    char c;
    while (scanf(" %c", &c) && c != 'q');

    pthread_mutex_lock(&shm->mtx);
    shm->end = 1;
    pthread_cond_broadcast(&shm->not_empty);
    pthread_cond_broadcast(&shm->not_full);
    pthread_mutex_unlock(&shm->mtx);

    while (wait(NULL) > 0);

    close(fd);
    munmap(shm, sizeof(shm_t));
    shm_unlink(SHM_NAME);
    return 0;
}
