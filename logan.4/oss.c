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
#include <limits.h>
#include "structures.h"

#define SHAREDMEMKEY 0x1596


jmp_buf ctrlCjmp;
jmp_buf timerjmp;

/*Structure Id's*/
int pcbID;
int msqID;
int clocKID;


/*pointers to shared memory
 *  * structures*/
MessageQ *msq;
MemoryClock *memoryClock;
ProcessCtrlBlk *pcbTable;

void detachSharedMemory(long *, int, char *);

long *connectToSharedMemory(int *, char *, int);

pid_t launchProcess(char *, int);

void ctrlCHandler();

void timerHandler();

unsigned int startTimer(int);

ProcessCtrlBlk *connectPcb(int, char *);

int findTableLocation(const int[], int);

Queue *createQueue(unsigned int);

void enqueue(Queue *queue, int item);

void initializePCB(int);

int main(int argc, char *argv[]) {

    const unsigned  int maxTimeBetweenNewProcsSecs = 1;
    const unsigned  int maxTimeBetweenNewProcsNS = 0;

    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];
    FILE *input;
    FILE *output;
    FILE *outputLog;
    int lineCounter = 0;
    int seconds = 3;
    int statusCode = 0;
    int childLogicalID = 0;
    int sharedMemoryId;
    int bitVector[18] = {0};
    int openTableLocation;
    int runningChildren = 0;
    int pctLocation = 0;
    long *sharedMemoryPtr;
    int totalPCBs = 18;
    Queue *queue1 = createQueue(18);


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

        /*Detach process control block shared memory*/
        if(shmdt(pcbTable) == -1){
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear pcb shared memory*/
        shmctl(pcbID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }
    if (setjmp(timerjmp) == 1) {


        /*Detach process control block shared memory*/
        if(shmdt(pcbTable) == -1){
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear pcb shared memory*/
        shmctl(pcbID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }

    startTimer(seconds);

    /*Connect and create pcbtable in shared memory*/
    pcbTable = connectPcb(totalPCBs, errorString);


    /*Start launching processes, find an open space in pcb table first*/
    if((openTableLocation = findTableLocation(bitVector, totalPCBs)) != - 1) {

        printf("open table locations: %d\n", openTableLocation);

        /*Launch process*/
        pid = launchProcess(errorString, openTableLocation);

        /*Queue it into the first queue*/
        enqueue(queue1, openTableLocation);

        /*Show that a process is occupying a space in
 *          * the bitvector which will directly relate to the pcb table*/
        bitVector[openTableLocation] = 1;

        wait(&statusCode);
    }




    /*detach pcb shared memory*/
    if(shmdt(pcbTable) == -1){
        snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Clear pcb shared memory*/
    shmctl(pcbID, IPC_RMID, NULL);


    startTimer(seconds);

}

/*Connect to a portion of shared memory for the process control blocks, create enough
 *  * space to hold 18 process control blocks*/
ProcessCtrlBlk *connectPcb(int totalpcbs, char *error) {
    char errorArr[200];

    pcbID = shmget(SHAREDMEMKEY, totalpcbs * sizeof(ProcessCtrlBlk), 0777 | IPC_CREAT);
    if (pcbID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space", errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to attach to shared memory ", errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return pcbTable;
}


/*Function to find an open location in the bit vector which will correspond to the spot
 *  * in the pcb table*/
int findTableLocation(const int bitVector[], int totalPCBs){
    int i;

    for(i = 0; i < totalPCBs; i++){
        if(bitVector[i] == 0){
            return i;
        }
    }

    return -1;
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
pid_t launchProcess(char *error, int openTableLocation) {
    char errorArr[200];
    pid_t pid;

    char tableLocationString[20];

    snprintf(tableLocationString, 20, "%d", openTableLocation);


    /*Fork process and capture pid*/
    pid = fork();

    if (pid == -1) {
        snprintf(errorArr, 200, "\n\n%s Fork execution failure", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /*Set initial values of process control block of process*/
        initializePCB(openTableLocation);

        /*executes files, Name of file to be execute, list of args following,
 *          * must be terminated with null*/
        execl("./userProcess", "userProcess", tableLocationString,  NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
        
    } else
        return pid;

}

void initializePCB(int tableLocation){
    pcbTable[tableLocation].pid = getpid();
    pcbTable[tableLocation].ppid = getppid();
    pcbTable[tableLocation].uId = getuid();
    pcbTable[tableLocation].gId = getgid();

    printf("oss: Process pid: %d\n ppid: %d\n gid: %d\n uid: %d\n\n",
           pcbTable[tableLocation].pid, pcbTable[tableLocation].ppid, pcbTable[tableLocation].gId, pcbTable[tableLocation].uId);
}

/**Implementation from geeksforgeeks.com**/
Queue *createQueue(unsigned capacity) {
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (int *) malloc(queue->capacity * sizeof(int));
    return queue;
}

/*Queue is full when size becomes equal to the capacity*/
int isFull(Queue *queue) { return (queue->size == queue->capacity); }

/* Queue is empty when size is 0*/
int isEmpty(Queue *queue) { return (queue->size == 0); }

/* Function to add an item to the queue.
 *  * It changes rear and size*/
void enqueue(Queue *queue, int item) {
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    printf("%d enqueued to queue\n", item);
}

/*Function to remove an item from queue.
 *  * It changes front and size*/
int dequeue(Queue *queue) {
    if (isEmpty(queue))
        return INT_MIN;
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

/* Function to get front of queue*/
int front(Queue *queue) {
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->front];
}

/* Function to get rear of queue*/
int rear(Queue *queue) {
    if (isEmpty(queue))
        return INT_MIN;
    return queue->array[queue->rear];
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
