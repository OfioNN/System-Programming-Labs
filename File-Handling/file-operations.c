#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>


int main(int argc, char *argv[]) {

    int fd1, fd2;

    // Zadanie 1 ----------------
    if (argc != 3) {
        printf("Użycie: %s <plik_wyjsciowy> <wzorzec>\n", argv[0]);
        exit(1);
    }
    // ----------------

    // Zadanie 2 ----------------
    if ((fd1 = open ("tmp.txt", O_RDONLY)) == -1) {
        perror ("\n Plik tmp.txt");
        exit (-1); 
    }
    // ----------------

    // Zadanie 3 ----------------
    if (access(argv[1], F_OK) == 0) {
        if (access(argv[1], W_OK) != 0) { 
            perror("Brak prawa zapisu"); 
            exit(1); 
        }
        fd2 = open(argv[1], O_WRONLY | O_TRUNC);
    } 
    else {
        fd2 = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }

    if (fd2 == -1) {
        perror("open wyj"); 
        exit(1); 
    }
    // ----------------

    // Zadanie 4 ----------------
    size_t txt_len = 0;
    char *txt = read_all_fd(fd1, &txt_len);

    writestr(fd2, "zadanie 1:\n");
    writestr(STDOUT_FILENO, "zadanie 1:\n");
    const int l1[] = {3,5,9};
    dump_lines(txt, txt_len, l1, sizeof(l1)/sizeof(l1[0]), 1, fd2, 0);
    // ----------------
    
    // Zadanie 5 ----------------
    writestr(fd2, "zadanie 2:\n");
    const int l2[] = {2,4,7};
    dump_lines(txt, txt_len, l2, sizeof(l2)/sizeof(l2[0]), 0, fd2, 1);
    // ----------------


    // Zadanie 6 ----------------
    writestr(fd2, "zadanie 3:\n");
    for (size_t i = 0; i < txt_len; ++i) {
        unsigned char c = (unsigned char)txt[i];
        if (isdigit(c)) write_all(fd2, &c, 1);
    }
    writestr(fd2, "\n");
    // ----------------

    // Zadanie 7 ----------------
    writestr(fd2, "zadanie 4:\n");
    size_t nlen = strlen(needle);
    if (nlen == 0) {
        writestr(fd2, "brak (pusty wzorzec)\n");
    } else {
        int any = 0;
        // mały bufor jak w przykładzie, skan bajt po bajcie na całym buforze w pamięci
        for (size_t i = 0; i + nlen <= txt_len; ++i) {
            if (memcmp(txt + i, needle, nlen) == 0) {
                any = 1;
                char line[64];
                int L = snprintf(line, sizeof(line), "match at offset %zu\n", i);
                write_all(fd2, line, (size_t)L);
            }
        }
        if (!any) writestr(fd2, "brak\n");
    }
    // ----------------

    // Zadanie 8 ----------------
    struct stat st;
    if (fstat(fd2, &st) == -1) { perror("fstat"); free(txt); close(fd1); close(fd2); exit(1); }
    {
        char line[128];
        int L = snprintf(line, sizeof(line), "Rozmiar pliku \"%s\": %jd bajtów\n",
                         out_path, (intmax_t)st.st_size);
        write_all(STDOUT_FILENO, line, (size_t)L);
    }
    // ----------------


    // Zadanie 9 ----------------
    if (fchmod(fd2, 0600) == -1) { perror("fchmod 0600"); }
    // ----------------

    // Zadanie 10 ----------------
    writestr(STDOUT_FILENO, "\n");

    free(txt);
    close(fd1);
    close(fd2);
    return EXIT_SUCCESS;
    // ----------------


}