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

/*Shared memory structure*/
struct sharedMemoryContainer {
    int seconds;
    long nanoseconds;
    int* childArray;
};

/*Create linked list to store Child pids*/
struct Node {
    int data;
    struct Node* next;
};

jmp_buf ctrlCjmp;
jmp_buf timerjmp;

void displayUsage();
struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer*, int*, char*);
void detachSharedMemory(struct sharedMemoryContainer*, int, char*);
pid_t launchProcess(char *, int, int);
unsigned int startTimer(int);
void push(struct Node**, int );
void deleteNode(struct Node**, int);
void ctrlCHandler();
void timerHandler();

int main(int argc, char* argv[]) {

    struct sharedMemoryContainer* shMemptr;
    int c;
    int seconds = 2;
    int prime = 100;
    int childLogicalID = 0;
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
    struct Node* childPIDs = NULL;


    /*Able to represent process ID*/
    pid_t pid = 0;

    /*Catch ctrl+c signal and handle with
 *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler*/
    signal (SIGALRM, timerHandler);

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


    startTimer(seconds);

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

    /*Launch a child process*/
    childLogicalID += 1;
    pid = launchProcess(errorString, childLogicalID, prime);
    push(&childPIDs, pid);

    /*Jump back to main enviroment where children are launched
 *      * but before the wait*/
    if(setjmp(ctrlCjmp) == 1){

        while(childPIDs != NULL){
            pid = childPIDs->data;
            kill(pid, SIGKILL);
            deleteNode(&childPIDs, pid);
        }

        shmdt(sharedMemoryPtr);
        shmctl(sharedMemoryId, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }
    if(setjmp(timerjmp) == 1){

        while(childPIDs != NULL){
            pid = childPIDs->data;
            kill(pid, SIGKILL);
            deleteNode(&childPIDs, pid);
        }

        shmdt(sharedMemoryPtr);
        shmctl(sharedMemoryId, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }


    pid = wait(&statusCode);
    printf("child exited after wait\n");
    deleteNode(&childPIDs, pid);


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

/*Alarm function defined by GNU*/
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
