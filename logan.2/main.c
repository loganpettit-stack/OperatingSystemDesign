#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHMEMKEY 0x1596

/*Shared memory structure*/
struct sharedMemoryContainer {
    int seconds;
    long nanoseconds;
};

void displayUsage();
struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer*, int*, char*);
void detachSharedMemory(struct sharedMemoryContainer*, int, char*);


int main(int argc, char* argv[]) {

    struct sharedMemoryContainer* shMemptr;
    int c;
    char* executable = strdup(argv[0]);
    char* errorString = strcat(executable, ": Error: ");
    char* Nerror;
    char* Serror;
    char* Berror;
    char* Ierror;
    int sharedMemoryId;
    struct sharedMemoryContainer* sharedMemoryPtr;


    if(argc > 1) {

        /*Use getopt to parse command arguements, : means option
 *          * requires second argument :: means second arguement is
 *                   * optional*/
        while ((c = getopt(argc, argv, "hn:s:b:i:o:")) != -1)
            switch (c) {

                /*Help option*/
                case 'h' :
                    printf("\nHelp Option Selected [-h]\n\n ");
                    displayUsage();
                    exit(EXIT_SUCCESS);


                /*indicate the max total of child process oss will
 *                  * create*/
                case 'n':
                    if(!isdigit(*optarg)){
                        Nerror = strcat(errorString, " [-n] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Nerror);
                        displayUsage();
                        exit(EXIT_FAILURE);

                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*indicate the number of children allowed to exist in the
 *                  * system at the same time*/
                case 's' :
                    if(!isdigit(*optarg)){
                        Serror = strcat(errorString, " [-s] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Serror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*start of the sequence of numbers to bes tested for primality*/
                case 'b':
                    if(!isdigit(*optarg)){
                        Berror = strcat(errorString, " [-b] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Berror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*increment between numbers that we test*/
                case 'i':
                    if(!isdigit(*optarg)){
                        Ierror = strcat(errorString, " [-i] option requires a digit arguement\n");
                        fprintf(stderr, "%s", Ierror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("optarg is:  %s \n", optarg);
                    }

                    break;

                /*-o filename output file */
                case 'o':
                    printf("o selected\n");

                    break;
            }
    }

    /*Obtain pointer to shared memory*/
    sharedMemoryPtr = connectToSharedMemory(sharedMemoryPtr, &sharedMemoryId, errorString);


    /*Fill shared memory structure*/
    sharedMemoryPtr->seconds = 10;
    sharedMemoryPtr->nanoseconds = 10;
    
    printf("%d:%d", sharedMemoryPtr->seconds, sharedMemoryPtr->nanoseconds);


    /*Detach pointer to shared memory*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);


}

void displayUsage(){
    fprintf(stderr, "This program will be executed ..\n");
    fprintf(stderr, "-n x Indicate the maximum total of child processes oss will ever create. (Default 4)\n"
                    "-s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n"
                    "-b B Start of the sequence of numbers to be tested for primality\n"
                    "-i I Increment between numbers that we test\n"
                    "-o filename Output file.\n");

}

struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer* sharedMemoryPtr, int* sharedMemoryID, char* error){
    char errorArr[200];

    /*Get shared memory Id using shmget*/
    *sharedMemoryID = shmget(SHMEMKEY, sizeof(struct sharedMemoryContainer), 0666 | IPC_CREAT);

    /*Error check shmget*/
    if (sharedMemoryID == (void *) -1) {
        snprintf(errorArr, 100, "\n\n%s PID: %ld Failed to create shared memory ", error,
                 (long) getpid());
        perror(errorArr);
        fprintf(stderr, "\tVale of errno: %d\n\n", errno);
        exit(EXIT_FAILURE);
    }

    /*Attach to shared memory using shmat*/
    sharedMemoryPtr = shmat(*sharedMemoryID, NULL, 0);

    /*Error check shmat*/
    if(sharedMemoryPtr == (void*) -1){
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error,
                 (long) getpid());
        perror(errorArr);
        fprintf(stderr, "\tVale of errno: %d\n\n", errno);
        exit(EXIT_FAILURE);
    }

    return sharedMemoryPtr;
}


void detachSharedMemory (struct sharedMemoryContainer* sharedMemoryPtr, int sharedMemoryId, char* error){
    char errorArr[200];

    if(shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error,
                   (long) getpid());
        perror(errorArr);
        fprintf(stderr, "\tVale of errno: %d\n\n", errno);
        exit(EXIT_FAILURE);
    }

    /* Clearing shared memory related to clockId */
    shmctl(sharedMemoryId, IPC_RMID, NULL);
}
