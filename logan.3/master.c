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



    /*Able to represent process ID*/
    pid_t pid = 0;

    /*Catch ctrl+c signal and handle with
 *      * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler */
    signal(SIGALRM, timerHandler);


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

    if (output == NULL) {
        fprintf(stderr, "output file could not be opened");
        return 1;
    }

    int x;
    for (x = 1; x < 17; x++) {
        fprintf(output, "%d\n", x);
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

    printf("line counter: %d\n", lineCounter);


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
        /* different way to get digit from file
 *           fscanf(input, "%d", &d);
 *                     printf("whole line %d\n", d);*/

        fgets(line, sizeof(line), input);
        fileDigit = strtol(line, &endPointer, 10);
        /*printf("whole line: %ld\n", fileDigit);*/

        sharedMemoryPtr[i] = fileDigit;
    }

    fclose(input);

    /*test shared memory array
 *     int t;
 *         for(t = 0; t < fileData; t++){
 *                 printf("shared memory pointer: %ld\n", sharedMemoryPtr[t]);
 *                     }*/


    /*n/2 summation launch*/
    int childrenToLaunch = fileData / 2;
    int index = 0;
    int numberDistance = 2;


    /* printf("children to launch before %d\n", childrenToLaunch);*/


    outputLog = fopen(output_log, "w");
    fprintf(outputLog, "\nSummation of numbers splitting them into n/2 groups: \n\n");
    char *pidString = "PID";
    char *indexString = "Index";
    char *addedString = "Size";
    int flag = 1;

    if (outputLog == NULL) {
        fprintf(stderr, "adder_log could not be openend");
        return 1;
    }

    fprintf(outputLog, "%-10s %-5s %-5s\n", pidString, indexString, addedString);

    fclose(outputLog);


    while (childrenToLaunch > 0) {

        runningChildren = childrenToLaunch;

        while (runningChildren > 0) {


            /*Flag indicates a layer is ready to launch*/
            if(flag == 1) {
                int l;
                for (l = 0; l < childrenToLaunch; l++) {
                    pid = launchProcess(errorString, childLogicalID, index, numbersToAdd);
                    push(&childPIDs, pid);
                    index += numberDistance;
                    childLogicalID += 1;
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

            /*if there are no more running children launch
 *              * the next layer*/
            if(runningChildren == 0){
                flag = 1;
            }
        }

        /*handle shifting of elements to begining indexs of
 *          * shared memory*/
        sharedMemoryPtr[1] = sharedMemoryPtr[numberDistance];

        int k;
        for (k = 2; k < childrenToLaunch; k++) {
            sharedMemoryPtr[k] = sharedMemoryPtr[k * 2];
        }

        index = 0;
        childrenToLaunch /= 2;
        fileData /= 2;

        /*
 *           int t;
 *                     for (t = 0; t < 8; t++) {
 *                                   printf("MASTER: shared memory pointer: %ld\n", sharedMemoryPtr[t]);
 *                                             }*/
    }

    printf("\nn/2 Summed of data in file: %ld\n\n", sharedMemoryPtr[0]);


/*re load data into shared memory
 *     input = fopen(inputFile, "r");
 *         if (input == NULL) {
 *                 fprintf(stderr,
 *                                 "Input file could not be opened");
 *                                         return 1;
 *                                             }
 *
 *
 *                                                 int j;
 *                                                     for (
 *                                                                 j = 0;
 *                                                                             j < lineCounter;
 *                                                                                         j++) {
 *                                                                                                 fgets(line,
 *                                                                                                               sizeof(line), input);
 *                                                                                                                       fileDigit = strtol(line, &endPointer, 10);
 *
 *                                                                                                                               sharedMemoryPtr[j] =
 *                                                                                                                                               fileDigit;
 *                                                                                                                                                   }
 *
 *                                                                                                                                                       fclose(input);*/


/*test shared memory array
 *     int t;
 *         for (
 *                     t = 0;
 *                                 t < fileData;
 *                                             t++) {
 *                                                     printf("shared memory pointer: %ld\n", sharedMemoryPtr[t]);
 *                                                         }
 *
 *
 *                                                             outputLog = fopen(output_log, "a");
 *                                                                 fprintf(outputLog,
 *                                                                             "\nSummation of numbers splitting them into n/log(n) groups: \n\n");
 *
 *                                                                                 if (outputLog == NULL) {
 *                                                                                         fprintf(stderr,
 *                                                                                                         "adder_log could not be openend");
 *                                                                                                                 return 1;
 *                                                                                                                     }
 *
 *                                                                                                                         fprintf(outputLog,
 *                                                                                                                                     "%-10s %-5s %-5s\n", pidString, indexString, addedString);
 *
 *                                                                                                                                         fclose(outputLog);*/


/*Log summation launch
 *     printf("\n\n\nRUNNING LOG SUMMATION: \n");
 *
 *         double logChildrenToLaunch = 0;
 *             fileData = originalFileData;
 *                 logChildrenToLaunch = ceil((double) fileData / log2((double) fileData));
 *                     index = 0;
 *                         double logNumberDistance;
 *                             logNumberDistance = log2(fileData);
 *                                 int logNumbersToAdd = (int) logNumberDistance;
 *                                     int loopCounter = 0;
 *
 *                                         printf("\n log number distance %f\n", logNumberDistance);
 *                                             printf("\nlog children to launch %f\n", logChildrenToLaunch);
 *
 *
 *                                                 while (logChildrenToLaunch > 0) {
 *
 *                                                         while (index < fileData) {
 *
 *                                                                     pid = launchProcess(errorString, childLogicalID, index, logNumbersToAdd);
 *                                                                                 push(&childPIDs, pid);
 *                                                                                             index += (int)
 *                                                                                                                 logNumberDistance;
 *                                                                                                                             childLogicalID += 1;
 *
 *                                                                                                                                         pid = wait(&statusCode);
 *
 *
 *                                                                                                                                                     if (pid > 0) {
 *                                                                                                                                                                     fprintf(stderr,
 *                                                                                                                                                                                             "Child with PID %ld exited with status 0x%x\n",
 *                                                                                                                                                                                                                     (long) pid, statusCode);
 *                                                                                                                                                                                                                                 }
 *                                                                                                                                                                                                                                         }
 *                                                                                                                                                                                                                                                 */
/*handle shifting of elements
 *         sharedMemoryPtr[1] = sharedMemoryPtr[(int) logNumberDistance];
 *
 *                 int k;
 *                         for (
 *                                         k = 2;
 *                                                         k < logChildrenToLaunch;
 *                                                                         k++) {
 *                                                                                     sharedMemoryPtr[k] = sharedMemoryPtr[k * (int) logNumberDistance];
 *                                                                                             }
 *
 *                                                                                                 */
/*Set up next layer to be added
 *         index = 0;
 *                 logChildrenToLaunch = ceil(logChildrenToLaunch / 2);
 *                     */
/*handle infinite loop caused by ceil of 1/2 never being 0
 *         if (logChildrenToLaunch == 1) {
 *                     if (loopCounter == 1) {
 *                                     logChildrenToLaunch = 0;
 *                                                 }
 *
 *                                                             loopCounter += 1;
 *                                                                     }
 *
 *
 *                                                                             fileData /=
 *                                                                                             logNumbersToAdd;
 *                                                                                                     logNumberDistance = 2;
 *                                                                                                             logNumbersToAdd = 2;
 *                                                                                                                 */
/* printf("LAYER FINISHED children to launch %f\n\n\n", logChildrenToLaunch);
 *
 *  int t;
 *   for (t = 0; t < 64; t++) {
 *        printf("MASTER: shared memory pointer: %ld\n", sharedMemoryPtr[t]);
 *         }
 *             }
 *
 *
 *                 printf("\nLog summed of data in file: %ld\n\n", sharedMemoryPtr[0]);*/


    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString
    );


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
