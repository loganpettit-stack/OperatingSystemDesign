#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

    if (argv[0] != NULL && argv[1] != NULL) {
        printf("Child: %d launched", atoi(argv[1]));
        printf("prime number is: %d", atoi(argv[2]));
    } else {
        printf("Child arg erros");
    }

    return 0;
}
