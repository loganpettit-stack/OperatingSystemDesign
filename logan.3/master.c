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
#include <time.h>


#define SHAREDMEMKEY 0x1596
#define SEMAPHOREKEY 0x0852

struct Node {
    int data;
    struct Node *next;
};


jmp_buf ctrlCjmp;
jmp_buf timerjmp;


void detachSharedMemory(long *, int, char *);

long *connectToSharedMemory(int *, char *, int);

void push(struct Node **, int);

pid_t launchProcess(char *, int, int, int);

void deleteNode(struct Node **, int);

void ctrlCHandler();

void timerHandler();

unsigned int startTimer(int);


int main(int argc, char *argv[]) {


    char *inputFile = "input.dat";
    char *generatedFile = "input.dat";
    char *output_log = "adder_log";
    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char line[51] = {0};
    FILE *input;
    FILE *output;
    FILE *outputLog;
    int lineCounter = 0;
    int seconds = 100;
    int statusCode = 0;
    int childLogicalID = 0;
    int fileData;
    int numbersToAdd = 2;
    int originalFileData;
    int sharedMemoryId;
    int runningChildren = 0;
    long *sharedMemoryPtr;
    struct Node *childPIDs = NULL;
    time_t t;
    time(&t);
    time_t beforeTime;
    time_t afterTime;
    long time_past;
    int upper = 256;
    int lower = 0;

    /*Able to represent process ID*/
    pid_t pid = 0;

    /*Catch ctrl+c signal and handle with
 *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler */
    signal(SIGALRM, timerHandler);


    /*Jump back to main enviroment where Concurrentchildren are launched
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

    if (output == NULL) {
        fprintf(stderr, "output file could not be opened");
        return 1;
    }

    int x;
    int randomNumber;
    for (x = 1; x <=64 ; x++) {
        randomNumber = (rand() % (upper - lower + 1)) + lower;
        fprintf(output, "%d\n", randomNumber);
    }


    fclose(output);

    /*check how many lines is in file*/
    input = fopen(inputFile, "r");

    if (input == NULL) {
        fprintf(stderr, "Input file could not be opened");
        return 1;
    }

    while (fgets(line, sizeof(line), input) != NULL) {
        lineCounter += 1;

    }

    originalFileData = lineCounter;
    fileData = originalFileData;

    fclose(input);

    printf("Amount of lines in file: %d\n", lineCounter);


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

    char *endPointer;
    long fileDigit = 0;
    int i;
    for (i = 0; i < lineCounter; i++) {
        fgets(line, sizeof(line), input);
        fileDigit = strtol(line, &endPointer, 10);

        sharedMemoryPtr[i] = fileDigit;
    }

    fclose(input);

    /*n/2 summation launch*/
    int childrenToLaunch = fileData / 2;
    int originalChildrenToLaunch = childrenToLaunch;
    int index = 0;
    int numberDistance = 2;
    int flag = 1;
    int Concurrentchildren;
    int childrenInLayer = childrenToLaunch;

    if (childrenToLaunch < 10) {
        Concurrentchildren = childrenToLaunch;
    } else {
        Concurrentchildren = 10;
    }



    outputLog = fopen(output_log, "w");
    if (outputLog == NULL) {
        fprintf(stderr, "adder_log could not be openend");
        return 1;
    }
    fprintf(outputLog, "\nSummation of numbers splitting them into n/2 groups: \n");
    char *pidString = "PID";
    char *indexString = "Index";
    char *addedString = "Size";


    fprintf(outputLog, "%-10s %-5s %-5s\n", pidString, indexString, addedString);

    fclose(outputLog);

    time(&beforeTime);

    printf("\n\nRUNNING N/2 SUMMATION:\n");
    printf("Initial Children to launch summing groups of 2: %d\n", childrenToLaunch);
    printf("Calculation start time: %s\n\n\n", ctime(&beforeTime));

    /*Launch n/2 processes*/
    while (childrenToLaunch > 0) {


         /*Waiting for all children in layer to launch and finish*/
        while (childrenInLayer > 0) {

            runningChildren = Concurrentchildren;

            /*Launch processes concurrently and loop wait until finished*/
            while (runningChildren > 0) {

                /*If flag is raised more children are ready
 *                  * to be launched*/
                if (flag == 1) {
                    int l;
                    for (l = 0; l < Concurrentchildren; l++) {
                        pid = launchProcess(errorString, childLogicalID, index, numbersToAdd);
                        push(&childPIDs, pid);
                        index += numberDistance;
                        childLogicalID += 1;
                        childrenToLaunch -=1;
                    }
                    flag = 0;
                }

                pid = wait(&statusCode);
                if (pid > 0) {
                    fprintf(stderr, "Child with PID %ld exited with status 0x%x\n",
                            (long) pid, statusCode);

                    runningChildren -= 1;
                    deleteNode(&childPIDs, pid);
                }
            }

            flag = 1;
            childrenInLayer = childrenToLaunch;

            /*if we have launched 10 processes and
 *             * waited for those to finish launch the next set*/
            if (childrenToLaunch < 10 ) {
                Concurrentchildren = childrenToLaunch;
            } else {
                Concurrentchildren = 10;
            }
        }

        /*handle shifting of elements to begining indexs of
 *          * shared memory*/
        sharedMemoryPtr[1] = sharedMemoryPtr[numberDistance];

        int k;
        for (k = 2; k < originalChildrenToLaunch; k++) {
            sharedMemoryPtr[k] = sharedMemoryPtr[k * 2];
        }

        index = 0;
        originalChildrenToLaunch /= 2;
        childrenToLaunch = originalChildrenToLaunch;
        childrenInLayer = childrenToLaunch;
        fileData /= 2;

        if (childrenToLaunch < 10) {
            Concurrentchildren = childrenToLaunch;
        } else {
            Concurrentchildren = 10;
        }
    }

    time(&afterTime);
    time_past = afterTime - beforeTime;

    outputLog = fopen(output_log, "a");
    if (outputLog == NULL) {
        fprintf(stderr, "adder_log could not be openend");
        return 1;
    }

    printf("\nn/2 Summed of data in file: %ld\n\n", sharedMemoryPtr[0]);
    fprintf(outputLog, "\nFile data summed to: %ld\n", sharedMemoryPtr[0]);
    fprintf(outputLog, "Time taken to execute: %ld seconds\n", time_past);

    fclose(outputLog);


    /*re load data into shared memory*/
    input = fopen(inputFile, "r");
    if (input == NULL) {
        fprintf(stderr,
                "Input file could not be opened");
        return 1;
    }

    int j;
    for (j = 0; j < lineCounter; j++) {
        fgets(line, sizeof(line), input);
        fileDigit = strtol(line, &endPointer, 10);
        sharedMemoryPtr[j] = fileDigit;
    }

    fclose(input);

    outputLog = fopen(output_log, "a");
    fprintf(outputLog,
            "\nSummation of numbers splitting them into n/log(n) groups: \n\n");

    if (outputLog == NULL) {
        fprintf(stderr,
                "adder_log could not be openend");
        return 1;
    }

    fprintf(outputLog,
            "%-10s %-5s %-5s\n", pidString, indexString, addedString);

    fclose(outputLog);


    /*Log summation launch */
    double logChildrenToLaunch = 0;
    fileData = originalFileData;
    logChildrenToLaunch = ceil((double) fileData / log2((double) fileData));
    originalChildrenToLaunch = (int)logChildrenToLaunch;
    index = 0;
    double logNumberDistance;
    logNumberDistance = log2(fileData);
    int logNumbersToAdd = (int) logNumberDistance;
    int loopCounter = 0;
    childrenInLayer = logChildrenToLaunch;

    /*Set children to launch*/
    if (logChildrenToLaunch < 10) {
        Concurrentchildren = (int)logChildrenToLaunch;
    } else {
        Concurrentchildren = 10;
    }

    printf("\n\n\n\nRUNNING N/LOG(N) SUMMATION: \n");
    printf("Number group sizes:  %f\n", logNumberDistance);
    printf("Initial children to launch:  %f\n", logChildrenToLaunch);
    flag = 1;

    time(&beforeTime);

    printf("Calculation start time: %ld\n\n\n", beforeTime);

    /*loop to launch layers of processes*/
    while (logChildrenToLaunch > 0) {

        while(childrenInLayer > 0) {
            runningChildren = Concurrentchildren;

            /*loop to catch all launched Concurrentchildren*/
            while (runningChildren > 0) {


                if (flag == 1) {
                    int o;
                    for (o = 0; o < Concurrentchildren; o++) {
                        pid = launchProcess(errorString, childLogicalID, index, logNumbersToAdd);
                        push(&childPIDs, pid);
                        index += (int) logNumberDistance;
                        childLogicalID += 1;
                        logChildrenToLaunch -= 1;
                    }
                    flag = 0;
                }

                pid = wait(&statusCode);
                if (pid > 0) {
                    fprintf(stderr, "Child with PID %ld exited with status 0x%x\n",
                            (long) pid, statusCode);

                    runningChildren -= 1;
                    deleteNode(&childPIDs, pid);
                }
            }

            /*Allow processes to be launched again*/
            flag = 1;
            childrenInLayer = (int)logChildrenToLaunch;

            /*if we have launched 10 processes and
 *              * waited for those to finish launch the next set*/
            if (logChildrenToLaunch < 10 ) {
                Concurrentchildren = (int)logChildrenToLaunch;
            } else {
                Concurrentchildren = 10;
            }
        }

        /*handle shifting of elements*/
        sharedMemoryPtr[1] = sharedMemoryPtr[(int) logNumberDistance];

        int k;
        for (k = 2; k < originalChildrenToLaunch; k++) {
            sharedMemoryPtr[k] = sharedMemoryPtr[k * (int) logNumberDistance];
        }

        /*Zero out the rest of the shared memory to allow proper adding*/
        int u;
        for(u = originalChildrenToLaunch; u < fileData; u++){
            sharedMemoryPtr[u] = 0;
        }

        /*Set up next layer to be added*/
        index = 0;
        originalChildrenToLaunch = ceil((double)originalChildrenToLaunch/2);
        logChildrenToLaunch = originalChildrenToLaunch;



        /*handle infinite loop caused by ceil of 1/2 never being 0 */
        if (logChildrenToLaunch == 1) {
            if (loopCounter == 1) {
                logChildrenToLaunch = 0;
            }

            loopCounter += 1;
        }


        childrenInLayer = (int)logChildrenToLaunch;

        logNumberDistance = 2;
        logNumbersToAdd = 2;
    }

    time(&afterTime);
    time_past = afterTime - beforeTime;

    outputLog = fopen(output_log, "a");
    if (outputLog == NULL) {
        fprintf(stderr, "adder_log could not be openend");
        return 1;
    }

    printf("\nLog summed of data in file: %ld\n\n", sharedMemoryPtr[0]);
    fprintf(outputLog, "\nSummed data from file using n/log(n) groups: %ld\n", sharedMemoryPtr[0]);
    fprintf(outputLog, "Time taken to execute: %ld seconds\n", time_past);

    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);


}


/*Function to connect to shared memory and error check*/
long *connectToSharedMemory(int *sharedMemoryID, char *error, int lineCount) {
    char errorArr[200];
    long *sharedMemoryPtr;

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
    if (sharedMemoryPtr == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return sharedMemoryPtr;
}


/*Function to detach from shared memory and error check*/
void detachSharedMemory(long *sharedMemoryPtr, int sharedMemoryId, char *error) {
    char errorArr[200];

    if (shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Clearing shared memory related to clockId */
    shmctl(sharedMemoryId, IPC_RMID, NULL);
}

/*Function to launch child processes and return the corresponding pid's*/
pid_t launchProcess(char *error, int childLogicalID, int index, int numbers) {
    char errorArr[200];
    pid_t pid;

    char childLogicalString[20];
    char indexString[20];
    char numberString[20];

    snprintf(childLogicalString, 20, "%d", childLogicalID);
    snprintf(indexString, 20, "%d", index);
    snprintf(numberString, 20, "%d", numbers);


    /*Fork process and capture pid*/
    pid = fork();

    if (pid == -1) {
        snprintf(errorArr, 200, "\n\n%s Fork execution failure", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /*executes files, Name of file to be execute, list of args following,
 *          * must be terminated with null*/
        execl("./bin_adder", "bin_adder", childLogicalString, indexString, numberString, NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    } else
        return pid;

}

/**Implementation from geeksforgeeks.com**/
/* Given a reference (pointer to pointer) to the head of a list
 *    and an int,  inserts a new node on the front of the list. */
void push(struct Node **head_ref, int new_data) {
    /* 1. allocate node */
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));

    /* 2. put in the data  */
    new_node->data = new_data;

    /* 3. Make next of new node as head */
    new_node->next = (*head_ref);

    /* 4. move the head to point to the new node */
    (*head_ref) = new_node;
}

/**Implementation from geeksforgeeks.com**/
/* Given a reference (pointer to pointer) to the head of a list
 *    and a key, deletes the first occurrence of key in linked list */
void deleteNode(struct Node **head_ref, int key) {
    /* Store head node*/
    struct Node *temp = *head_ref, *prev;

    /* If head node itself holds the key to be deleted*/
    if (temp != NULL && temp->data == key) {
        *head_ref = temp->next;   /* Changed head */
        free(temp);               /* free old head */
        return;
    }

    /* Search for the key to be deleted, keep track of the*/
    /* previous node as we need to change 'prev->next' */
    while (temp != NULL && temp->data != key) {
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
    if (setitimer(ITIMER_REAL, &new, &old) < 0)
        return 0;
    else
        return old.it_value.tv_sec;
}

/*Function handles ctrl c signal and jumps to top
 *  * of program stack enviroment*/
void ctrlCHandler() {
    char errorArr[200];
    snprintf(errorArr, 200, "\n\nCTRL+C signal caught, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(ctrlCjmp, 1);
}

/*Function handles timer alarm signal and jumps
 *  * to top of program stack enviroment*/
void timerHandler() {
    char errorArr[200];
    snprintf(errorArr, 200, "\n\ntimer interrupt triggered, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(timerjmp, 1);
}
