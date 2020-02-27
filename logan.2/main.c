#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>
#include <setjmp.h>
#include <sys/time.h>
#include <signal.h>

#define SHAREDMEMKEY 0x1596

/*Shared memory structure
 * struct sharedMemoryContainer {
 *     int seconds;
 *         long nanoseconds;
 *             int* childArray;
 *             };*/

/*Create linked list to store Child pids*/
struct Node {
    int data;
    struct Node* next;
};

jmp_buf ctrlCjmp;
jmp_buf timerjmp;

/*void detachSharedMemory(struct sharedMemoryContainer*, int, char*);*/
/*struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer*, int*, char*);*/
void detachSharedMemory(long*, int, char*);
long*  connectToSharedMemory(int*, char*, int);
void displayUsage();
pid_t launchProcess(char *, int, int);
unsigned int startTimer(int);
void push(struct Node**, int );
void deleteNode(struct Node**, int);
void ctrlCHandler();
void timerHandler();

int main(int argc, char* argv[]) {

    /* struct sharedMemoryContainer* sharedMemoryPtr;*/

    int c;
    int incrementer = 1;
    int childrenCounter = 0;
    int seconds = 2;
    int prime = 2;
    int childLogicalID = 1;
    int maxChildren = 4;
    int concurrentChildren = 2;
    int statusCode = 0;
    int runningChildren = 0;
    char* executable = strdup(argv[0]);
    char* errorString = strcat(executable, ": Error: ");
    char* Nerror;
    char* Serror;
    char* Berror;
    char* Ierror;
    char* filename = "output.log";
    FILE* fptr;
    int sharedMemoryId;
    long* sharedMemoryPtr;
    struct Node* childPIDs = NULL;

    /*Able to represent process ID*/
    pid_t pid = 0;

    /*Catch ctrl+c signal and handle with
 *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler*/
    signal (SIGALRM, timerHandler);


    /*Jump back to main enviroment where children are launched
 *      * but before the wait*/
    if (setjmp(ctrlCjmp) == 1) {

        while (childPIDs != NULL) {
            pid = childPIDs->data;
            kill(pid, SIGKILL);
            deleteNode(&childPIDs, pid);
        }

        shmdt(sharedMemoryPtr);
        shmctl(sharedMemoryId, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }
    if (setjmp(timerjmp) == 1) {

        while (childPIDs != NULL) {
            pid = childPIDs->data;
            kill(pid, SIGKILL);
            deleteNode(&childPIDs, pid);
        }

        shmdt(sharedMemoryPtr);
        shmctl(sharedMemoryId, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }

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
                        maxChildren = atoi(optarg);
                    }

                    break;

                /*indicate the number of children allowed to exist in the
 *                  * system at the same time*/
                case 's' :
                    if(!isdigit(*optarg)){
                        fprintf(stderr, "%s", Serror);
                        displayUsage();
                        exit(EXIT_FAILURE);
                    }
                    else {
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
                        incrementer = atoi(optarg);
                    }

                    break;

                /*-o filename output file */
                case 'o':
                    filename = optarg;
                    break;
            }
    }

    printf("Max children to be created: %d\n", maxChildren);
    printf("Max children to be concurently running: %d:\n", concurrentChildren);
    printf("Number to start prime check sequence: %d\n", prime);
    printf("Number to increment the prime sequence: %d\n", incrementer);
    printf("File to be output to: %s\n", filename);



    fptr = fopen(filename, "w");
    if (fptr == NULL)
    {
        fprintf(stderr, "\nError opend file\n");
        exit(1);
    }

    startTimer(seconds);



    /*Obtain pointer to shared memory*/
    sharedMemoryPtr = connectToSharedMemory( &sharedMemoryId, errorString, maxChildren);


    /*Initialize shared memory seconds and nanoseconds as well
 *      * as array for child processes*/
    int i;
    for(i = 0; i < maxChildren + 2; i++){
        sharedMemoryPtr[i] = 0;
    }

    sharedMemoryPtr[0] = 0;
    sharedMemoryPtr[1] = 0;

    printf("\nMain initial time is: %ld:%ld\n", sharedMemoryPtr[0], sharedMemoryPtr[1]);

    printf("\n");

    int m;
    for(m = 0; m < concurrentChildren; m++){
        pid = launchProcess(errorString, childLogicalID, prime);
        printf("\nLaunching child %ld at time %ld:%ld\n", (long)pid, sharedMemoryPtr[0], sharedMemoryPtr[1]);
        push(&childPIDs, pid);
        childLogicalID += 1;
        childrenCounter += 1;
        runningChildren += 1;
        prime += incrementer;
    }

    sharedMemoryPtr[1] = 10000;

    usleep(1000);

    sharedMemoryPtr[1] += 10000;

    while(childrenCounter < maxChildren + 1 && runningChildren > 0){

        if(sharedMemoryPtr[1] > 1000000000){
            sharedMemoryPtr[0] += 1;
        } else{
            sharedMemoryPtr[1] += 100000;
        }


        pid = wait(&statusCode);

        if (pid > 0) {
            printf("\nChild with PID %ld exited with status 0x%x at main time %ld:%ld \n",
                   (long) pid, statusCode, sharedMemoryPtr[0], sharedMemoryPtr[1]);

            runningChildren -= 1;

            deleteNode(&childPIDs, pid);

            if (childrenCounter < maxChildren) {
                pid = launchProcess(errorString, childLogicalID, prime);
                printf("\nLaunching child %ld at time %ld:%ld\n\n", (long)pid, sharedMemoryPtr[0], sharedMemoryPtr[1]);
                push(&childPIDs, pid);
                childLogicalID += 1;
                childrenCounter += 1;
                runningChildren += 1;
                prime += incrementer;
              /*  usleep(1000);*/


                if(sharedMemoryPtr[1] > 1000000000){
                    sharedMemoryPtr[0] += 1;
                } else{
                    sharedMemoryPtr[1] += 100000;
                }

                if(sharedMemoryPtr[1] > 1000000000){
                    sharedMemoryPtr[0] += 1;
                } else{
                    sharedMemoryPtr[1] += 1000000;
                }


            }
        }
    }


    printf("\nMain time at end of program: %ld:%ld \n", sharedMemoryPtr[0], sharedMemoryPtr[1]);

    printf("Array of primes:\n");
    int y;
    for(y = 2; y < maxChildren + 2; y++){
        printf("%ld  ", sharedMemoryPtr[y]);
    }


    /*Detach pointer to shared memory*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);


    printf("\n");

    return  0;
}

void displayUsage(){
    fprintf(stderr, "This program will execute with the following option using the defaults unless otherwise specified: \n");
    fprintf(stderr, "-n x Indicate the maximum total of child processes oss will ever create. (Default 4)\n"
                    "-s x Indicate the number of children allowed to exist in the system at the same time. (Default 2)\n"
                    "-b B Start of the sequence of numbers to be tested for primality (Default 2)\n"
                    "-i I Increment between numbers that we test (Default 1)\n"
                    "-o filename Output file (Default output.log)\n");

}

/*Function to connect to shared memory and error check*/
long* connectToSharedMemory( int* sharedMemoryID, char* error, int maxChildren){
    char errorArr[200];
    long* sharedMemoryPtr;

    /*Get shared memory Id using shmget
 *     *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(struct sharedMemoryContainer), 0777 | IPC_CREAT);*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(int) + sizeof(long) + sizeof(int) * maxChildren, 0644 | IPC_CREAT);

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
void detachSharedMemory (long* sharedMemoryPtr, int sharedMemoryId, char* error){
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

/**Implementation from geeksforgeeks.com**/
/* Given a reference (pointer to pointer) to the head of a list
 *    and an int,  inserts a new node on the front of the list. */
void push(struct Node** head_ref, int new_data)
{
    /* 1. allocate node */
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));

    /* 2. put in the data  */
    new_node->data  = new_data;

    /* 3. Make next of new node as head */
    new_node->next = (*head_ref);

    /* 4. move the head to point to the new node */
    (*head_ref)    = new_node;
}

/**Implementation from geeksforgeeks.com**/
/* Given a reference (pointer to pointer) to the head of a list
 *    and a key, deletes the first occurrence of key in linked list */
void deleteNode(struct Node **head_ref, int key)
{
    /* Store head node*/
    struct Node* temp = *head_ref, *prev;

    /* If head node itself holds the key to be deleted*/
    if (temp != NULL && temp->data == key)
    {
        *head_ref = temp->next;   /* Changed head */
        free(temp);               /* free old head */
        return;
    }

    /* Search for the key to be deleted, keep track of the*/
    /* previous node as we need to change 'prev->next' */
    while (temp != NULL && temp->data != key)
    {
        prev = temp;
        temp = temp->next;
    }

    /* If key was not present in linked list*/
    if (temp == NULL) return;

    /* Unlink the node from linked list */
    prev->next = temp->next;

    free(temp);  /* Free memory */
}

/*Timer function that sends alarm defined by GNU*/
unsigned int startTimer(int seconds) {
    struct itimerval old, new;
    new.it_interval.tv_usec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = 0;
    new.it_value.tv_sec = (long int) seconds;
    if (setitimer (ITIMER_REAL, &new, &old) < 0)
        return 0;
    else
        return old.it_value.tv_sec;
}

/*Function handles ctrl c signal and jumps to top
 *  * of program stack enviroment*/
void ctrlCHandler(){
    char errorArr[200];
    snprintf(errorArr, 200, "\n\nCTRL+C signal caught, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(ctrlCjmp, 1);
}

/*Function handles timer alarm signal and jumps
 *  * to top of program stack enviroment*/
void timerHandler(){
    char errorArr[200];
    snprintf(errorArr, 200, "\n\ntimer interrupt triggered, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(timerjmp, 1);
}
