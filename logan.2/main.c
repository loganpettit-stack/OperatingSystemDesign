#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

#define SHAREDMEMKEY 0x1596

/*Shared memory structure*/
struct sharedMemoryContainer {
    int seconds;
    long nanoseconds;
    int* childArray;
};

void displayUsage();
struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer*, int*, char*);
void detachSharedMemory(struct sharedMemoryContainer*, int, char*);
pid_t launchProcess(char *, int, int);

int main(int argc, char* argv[]) {

    struct sharedMemoryContainer* shMemptr;
    int c;
    int prime = 100;
    int childLogicalID = 5;
    int maxChildren = 4;
    int concurrentChildren = 0;
    int statusCode = 0;
    char* executable = strdup(argv[0]);
    char* errorString = strcat(executable, ": Error: ");
    char* Nerror;
    char* Serror;
    char* Berror;
    char* Ierror;
    int sharedMemoryId;
    struct sharedMemoryContainer* sharedMemoryPtr;

    /*Able to represent process ID*/
    pid_t pid = 0;


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
                        printf("Max number of child processes to be created is:  %s \n", optarg);
                        maxChildren = atoi(optarg);
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
                        printf("Max number of concurrent child processes is:  %s \n", optarg);
                        concurrentChildren = atoi(optarg);
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
                        prime = atoi(optarg);
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


    /*Initialize shared memory structure*/
    sharedMemoryPtr->seconds = 1;
    sharedMemoryPtr->nanoseconds = 0;
    sharedMemoryPtr->childArray = malloc(sizeof(int) * maxChildren);

    /*Initialize shared memory child array to all zeros*/
    int i;
    for(i = 0; i < maxChildren; i++){
        sharedMemoryPtr->childArray[i] = 0;
    }

    /*testing shared memory segment*/
    printf("Main initial time: %d:%ld \n", sharedMemoryPtr->seconds, sharedMemoryPtr->nanoseconds);
    int j;
    for(j = 0; j < maxChildren; j++){
        printf("Shared memory array values: %d\n", sharedMemoryPtr->childArray[j]);
    }

    /*Launch a child process*/
    childLogicalID += 1;
    pid = launchProcess(errorString, childLogicalID, prime);

    pid = wait(&statusCode);
    printf("child exited after wait\n");
    
    
    printf("Main time after child exit: %d:%ld \n", sharedMemoryPtr->seconds, sharedMemoryPtr->nanoseconds);


    /*Detach pointer to shared memory*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);

    return  0;
}

void displayUsage(){
    fprintf(stderr, "This program will be executed ..\n");
    fprintf(stderr, "-n x Indicate the maximum total of child processes oss will ever create. (Default 4)\n"
                    "-s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n"
                    "-b B Start of the sequence of numbers to be tested for primality\n"
                    "-i I Increment between numbers that we test\n"
                    "-o filename Output file.\n");

}

/*Function to connect to shared memory and error check*/
struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer* sharedMemoryPtr, int* sharedMemoryID, char* error){
    char errorArr[200];

    /*Get shared memory Id using shmget*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(struct sharedMemoryContainer), 0644 | IPC_CREAT);

    /*Error check shmget*/
    if (sharedMemoryID == (void *) -1) {
        snprintf(errorArr, 100, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Attach to shared memory using shmat*/
    sharedMemoryPtr = shmat(*sharedMemoryID, NULL, 0);

    /*Error check shmat*/
    if(sharedMemoryPtr == (void*) -1){
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return sharedMemoryPtr;
}

/*Function to detach from shared memory and error check*/
void detachSharedMemory (struct sharedMemoryContainer* sharedMemoryPtr, int sharedMemoryId, char* error){
    char errorArr[200];

    if(shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Clearing shared memory related to clockId */
    shmctl(sharedMemoryId, IPC_RMID, NULL);
}

/*Function to launch child processes and return the corresponding pid's*/
pid_t launchProcess(char* error, int childLogicalID, int prime){
    char errorArr[200];
    pid_t pid;

    char childLogicalString[20];
    char primeString[20];

    snprintf(childLogicalString, 20, "%d", childLogicalID);
    snprintf(primeString, 20, "%d", prime);


    /*Fork process and capture pid*/
    pid = fork();

    if(pid == -1){
        snprintf(errorArr, 200, "\n\n%s Fork execution failure", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    if(pid == 0){
        /*executes files, Name of file to be execute, list of args following,
 *          * must be terminated with null*/
        execl("./prime", "prime", childLogicalString, primeString, NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    } else
        return pid;

}


