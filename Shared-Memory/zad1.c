#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define SHM_NAME "/honey_shm"
#define SEM_MEM  "/sem_mem"
#define SEM_FILE "/sem_file"

typedef struct {
    int honey;
} shared_t;

void bee(shared_t *shm, sem_t *mem, sem_t *file, int fd) {
    srand(getpid());
    int visits = rand() % 100;
    int total = 0;

    for (int i = 0; i < visits; i++) {
        int add = rand() % 10;

        sem_wait(mem);
        shm->honey += add;
        int current = shm->honey;
        sem_post(mem);

        sem_wait(file);
        dprintf(fd,
            "Pszczoła_%d: zaniosłam %dg miodu, w ulu jest %dg\n",
            getpid(), add, current);
        sem_post(file);

        total += add;
        usleep((rand() % 100) * 1000);
    }
    exit(0);
}

void bear(shared_t *shm, sem_t *mem, sem_t *file, int fd) {
    srand(getpid());
    int visits = rand() % 10;
    int total = 0;

    for (int i = 0; i < visits; i++) {
        int eat = rand() % 50;

        sem_wait(mem);
        if (shm->honey < eat) eat = shm->honey;
        shm->honey -= eat;
        int current = shm->honey;
        sem_post(mem);

        sem_wait(file);
        dprintf(fd,
            "Miś_%d: zjadłem %dg miodu, w ulu jest %dg\n",
            getpid(), eat, current);
        sem_post(file);

        total += eat;
        usleep((rand() % 1000) * 1000);
    }
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Użycie: %s pszczoły misie\n", argv[0]);
        return 1;
    }

    int bees = atoi(argv[1]);
    int bears = atoi(argv[2]);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_t));
    shared_t *shm = mmap(NULL, sizeof(shared_t),
                         PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    shm->honey = 0;

    sem_t *sem_mem  = sem_open(SEM_MEM, O_CREAT, 0666, 1);
    sem_t *sem_file = sem_open(SEM_FILE, O_CREAT, 0666, 1);

    int fd = open("zad4_1_wyniki.txt",
                  O_CREAT | O_WRONLY | O_TRUNC, 0666);

    for (int i = 0; i < bees; i++)
        if (!fork()) bee(shm, sem_mem, sem_file, fd);

    for (int i = 0; i < bears; i++)
        if (!fork()) bear(shm, sem_mem, sem_file, fd);

    while (wait(NULL) > 0);

    close(fd);
    munmap(shm, sizeof(shared_t));
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MEM);
    sem_unlink(SEM_FILE);
    return 0;
}
