#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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
#define SEMAPHOREKEY 0x0852

struct Node {
    int data;
    struct Node* next;
};


jmp_buf ctrlCjmp;
jmp_buf timerjmp;


void detachSharedMemory(long*, int, char*);
long*  connectToSharedMemory(int*, char*, int);
void push(struct Node**, int );
void deleteNode(struct Node**, int);
void ctrlCHandler();
void timerHandler();
unsigned int startTimer(int);


int main(int argc, char* argv[]) {


    char* inputFile = "input.dat";
    char* generatedFile = "input.dat";
    char* output_log = "adder_log";
    char line[51] = {0};
    FILE* input;
    FILE* output;
    int lineCounter = 0;
    int seconds = 100;
    char* executable = strdup(argv[0]);
    char* errorString = strcat(executable, ": Error: ");
    int sharedMemoryId;
    long* sharedMemoryPtr;
    struct Node* childPIDs = NULL;



    /*Able to represent process ID*/
    pid_t pid = 0;

    /*Catch ctrl+c signal and handle with
 *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler */
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




    /*Generate input.dat*/
    output = fopen(generatedFile, "w");

    if(output == NULL){
        fprintf(stderr, "output file could not be opened");
        return 1;
    }

    int x;
    for(x = 0; x < 65; x++) {
        fprintf(output, "%d\n", x);
    }

    fclose(output);

    /*check how many lines is in file*/
    input = fopen(inputFile, "r");

    if (input == NULL) {
        fprintf(stderr, "Input file could not be opened");
        return 1;
    }

    while(fgets(line, sizeof(line), input) != NULL) {
        lineCounter += 1;

    }
    fclose(input);

    printf("%d", lineCounter);


    /*Start the  main timer*/
    startTimer(seconds);


    /*Connect to shared memory*/
    sharedMemoryPtr = connectToSharedMemory(&sharedMemoryId, errorString, lineCounter);


    /*Load data into shared memory */
    input = fopen(inputFile, "r");
    if (input == NULL) {
        fprintf(stderr, "Input file could not be opened");
        return 1;
    }

    char* endPointer;
    long fileDigit = 0;
    int i;
    for(i = 0; i < lineCounter; i++){
      /* different way to get digit from file
 *         fscanf(input, "%d", &d);
 *                 printf("whole line %d\n", d);*/

        fgets(line, sizeof(line), input);
        fileDigit = strtol(line, &endPointer, 10);
        printf("whole line: %ld\n", fileDigit);

        sharedMemoryPtr[i] = fileDigit;
    }


    /*test shared memory array*/
    int t;
    for(t = 0; t < lineCounter; t++){
        printf("shared memory pointer: %ld\n", sharedMemoryPtr[t]);
    }

    fclose(input);

    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);


}

/*Function to connect to shared memory and error check*/
long* connectToSharedMemory( int* sharedMemoryID, char* error, int lineCount){
    char errorArr[200];
    long* sharedMemoryPtr;

    /*Get shared memory Id using shmget*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(int) * lineCount, 0777 | IPC_CREAT);

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
