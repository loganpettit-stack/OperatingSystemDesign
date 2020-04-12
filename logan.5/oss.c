#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/msg.h>
#include <setjmp.h>
#include "Structures.h"

#define CLOCKKEY 0x3574
#define SEMAPHOREKEY 0x1470
#define MESSAGEKEY 0x3596
#define PCBKEY 0x1596
#define DESCRIPTORKEY 0x0752

jmp_buf fileLinejmp;
jmp_buf ctrlCjmp;
jmp_buf timerjmp;

MemoryClock *memoryClock;
Semaphore *semaphore;
MessageQ *mesq;
ResourceDiscriptor *resources;
ProcessCtrlBlk *pcbTable;
Queue *q;

int clocKID;
int pcbID;
int semaphoreID;
int descriptorID;
int msqID;
int bitVector[numPCB] = {0};
int maxResources;
int linesWritten = 0;
int deadlockDetected = 0;

MemoryClock *connectToClock(char *);
void connectToMessageQueue(char *);
ProcessCtrlBlk *connectPcb(int, char *);
void connectSemaphore(char *);
void connectDescriptor(char *);

void ctrlCHandler();
void timerHandler();
pid_t launchProcess(char *, int);
int isEmpty(Queue *);
Queue *createQueue(unsigned int);
void enqueue(Queue *, int);
int dequeue(Queue *);


int main(int argc, char *argv[]) {

    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];

    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);

    /*Get Pcb table from creating a 1D array
 *      * of process control blocks*/
    pcbTable = connectPcb(numPCB , errorString);

    /*Connect to resource descriptor*/
    connectDescriptor(errorString);

    /*Initialize semaphore*/
    sem_init(&(semaphore->mutex), 1, 1);



    /*Clear shared memory*/
    shmctl(clocKID, IPC_RMID, NULL);
    shmctl(pcbID, IPC_RMID, NULL);
    shmctl(semaphoreID, IPC_RMID, NULL);
    shmctl(descriptorID, IPC_RMID, NULL);
    msgctl(msqID, IPC_RMID, NULL);
    return 0;
}

/*Function to connect to shared memory and error check*/
MemoryClock *connectToClock(char *error) {
    char errorArr[200];
    long *sharedMemoryPtr;

    /*Get shared memory Id using shmget*/
    clocKID = shmget(CLOCKKEY, sizeof(memoryClock), 0777 | IPC_CREAT);
 
    /*Error check shmget*/
    if (clocKID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space for clock", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }


    /*Attach to shared memory using shmat*/
    memoryClock = shmat(clocKID, NULL, 0);


    /*Error check shmat*/
    if (memoryClock == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %d Failed to attach to shared memory clock", error, getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return memoryClock;
}

/*Connect to a portion of shared memory for the process control blocks, create enough
 *  * space to hold 18 process control blocks*/
ProcessCtrlBlk *connectPcb(int totalpcbs, char *error) {
    char errorArr[200];

    pcbID = shmget(PCBKEY, totalpcbs * sizeof(ProcessCtrlBlk), 0777 | IPC_CREAT);
    if (pcbID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space for process control block",
                 error, (long)getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to attach to shared memory process control block", 
                error, (long)getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return pcbTable;
}

/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {
    char errorArr[200];

    if ((msqID = msgget(MESSAGEKEY, 0777 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s Pid: %d Failed to connect to message queue", error, getpid());
        perror(errorArr);
        exit(1);
    }
}

void connectSemaphore(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((semaphoreID = shmget(SEMAPHOREKEY, sizeof(Semaphore), 0644 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for semaphore", error,
                 (long) getpid());
        perror(error);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if ((semaphore = (Semaphore *) shmat(semaphoreID, (void *) 0, 0)) == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for semaphore", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}

void connectDescriptor(char *error){
    char errorArr[200];

    /* Getting the semaphore ID */
    if((descriptorID = shmget(DESCRIPTORKEY, sizeof(ResourceDiscriptor), 0644 | IPC_CREAT)) == -1){
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for resource descriptor", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if((resources = (ResourceDiscriptor*)shmat(descriptorID, (void *)0, 0)) == (void *)-1){
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for resource descriptor", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
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

    /* printf("%d enqueued to queue\n", pcbTable[item].pid);*/
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
