#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("\n");
    system("echo -e \"\\e[0;32m	Witaj! Aktualnie: $(date)\\033[0m\"");
    printf("\n");
    return 0;
}
