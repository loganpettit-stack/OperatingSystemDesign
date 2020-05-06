#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/msg.h>
#include <setjmp.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/ipc.h>
#include "structures.h"


jmp_buf ctrlCjmp;
jmp_buf timerjmp;


MemoryClock *memoryClock;
Semaphore *semaphore;
MESQ mesq;
ProcessInfo pcbTable[MAX_CONCURRENT_PROCS];
Queue *frameQueue;

int clocKID;
int semaphoreID;
int msqID;
int bitVector[MAX_PROCS] = {0};
int second_chance[MEM_SIZE] = {0};
int totalLaunched = 0;
int runningProcesses = 0;
int pageFaultCount = 0;
int requestCount = 0;
int secondChanceFramePlace = 0;
int dirtyBit;

void initializePCB(int, int);

MemoryClock *connectToClock(char *);

void connectSemaphore(char *);

void connectToMessageQueue(char *);

void ctrlCHandler();

void timerHandler();

pid_t launchProcess(char *, int);

int isEmpty(Queue *);

Queue *createQueue(unsigned int);

void enqueue(Queue *, int);

int dequeue(Queue *);

unsigned int startTimer(int);

void addTime(long);

long getTimeInNanoseconds();

int findTableLocation();

void helpMessage(char *);

void resetFrame(Frame[MEM_SIZE]);

void memoryReferenced(Frame[MEM_SIZE], char *);

void pageFaultOccured(Frame[MEM_SIZE], char *);

void displayTable(Frame[MEM_SIZE]);

int secondChanceAlgorithm(Frame[MEM_SIZE]);

int main(int argc, char *argv[]) {

    char errorArr[200];
    char *exe = strdup(argv[0]);
    char *executable = strdup(argv[0]);
    char *errorString = strcat(exe, ": Error: ");
    int statusCode = 0;
    int tableLocation = 0;
    FILE *outFile;
    int seconds = 5;
    char *fileName = "output.dat";
    char c;
    long nextLaunchTimeNanos = 0;
    long currentTimeNanos = 0;
    bool pageFault = true;
    long displayTime = 0;
    pid_t pid = 0;
    frameQueue = createQueue(MEM_SIZE);
    srand(time(0));


    outFile = fopen(fileName, "w");


    /*Handle abnormal termination*/
    /*Catch ctrl+c signal and handle with ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *      * and handle with timer Handler */
    signal(SIGALRM, timerHandler);


    /*Jump back to main enviroment where Concurrent
 *      * children are launched but before the wait*/
    if (setjmp(ctrlCjmp) == 1) {

        int u;
        for (u = 0; u < MAX_CONCURRENT_PROCS; u++) {
            if (bitVector[u] == 1) {
                fprintf(stderr, "killing process %ld \n", pcbTable[u].pid);
                kill(pcbTable[u].pid, SIGKILL);
            }
        }

        printf("oss program ending total page faults %d total launched %d running procs %d\n", pageFaultCount,
               totalLaunched, runningProcesses);


        fclose(outFile);
        shmdt(memoryClock);
        shmdt(semaphore);
        shmctl(clocKID, IPC_RMID, NULL);
        shmctl(semaphoreID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }
    if (setjmp(timerjmp) == 1) {

        int u;
        for (u = 0; u < MAX_CONCURRENT_PROCS; u++) {
            if (bitVector[u] == 1) {
                fprintf(stderr, "killing process %ld \n", pcbTable[u].pid);
                kill(pcbTable[u].pid, SIGKILL);
            }
        }


        printf("oss program ending total page faults %d total launched %d running proces%d\n", pageFaultCount,
               totalLaunched, runningProcesses);

        fclose(outFile);
        shmdt(memoryClock);
        shmdt(semaphore);
        shmctl(clocKID, IPC_RMID, NULL);
        shmctl(semaphoreID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }


    /* Checking command line options */
    while ((c = getopt(argc, argv, "m:")) != -1)
        switch (c) {
            case 'm':
                dirtyBit = atoi(optarg);
                printf("dirty bit %d\n", dirtyBit);
                break;
            default:
                helpMessage(executable);
        }


    /*Connect structures and initialize*/
    connectToClock(errorString);
    connectToMessageQueue(errorString);
    connectSemaphore(errorString);

    memoryClock->seconds = 0;
    memoryClock->nanoseconds = 0;

    /*Initialize semaphore*/
    sem_init(&(semaphore->mutex), 1, 1);

    /*Set up frame table and pcb table*/
    Frame frameTable[MEM_SIZE];

    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        frameTable[i].occupiedBit = 0;
    }

    int j;
    for (i = 0; i < MAX_CONCURRENT_PROCS; i++) {
        for (j = 0; j < PT_SIZE; j++) {
            pcbTable[i].pageTable[j] = -1;
        }
    }

    startTimer(seconds);


    /*Calculate launch time for child process between
 *      * 1 and 500 milliseconds 1 mili is 1,000,000 nanos*/
    nextLaunchTimeNanos = 0;
    currentTimeNanos = getTimeInNanoseconds();
    displayTime = currentTimeNanos + 1000000000;

    while (true) {

        currentTimeNanos = getTimeInNanoseconds();
        if (currentTimeNanos >= displayTime) {
            displayTable(frameTable);
            displayTime = currentTimeNanos + 50000000000;
        }

        /*Terminate program if we have no running processes and
 *          * we have ran 100 processes*/
        if (totalLaunched >= 100 && runningProcesses == 0) {
            fprintf(stderr,
                    "\n\n%s: 100 processes have been ran and there are no longer anymore running processes, "
                    "program terminating\n", executable);
            break;
        }

        currentTimeNanos = getTimeInNanoseconds();

        /*printf("current launch time %ld and time to next launch %ld\n", currentTimeNanos, nextLaunchTimeNanos);*/
        /*Check to see if we are ready to launch */
        if (currentTimeNanos >= nextLaunchTimeNanos && runningProcesses < 18 && totalLaunched < 100) {

            /*Find an open location in the bit vector and launch*/
            if ((tableLocation = findTableLocation(bitVector)) != -1) {
                pid = launchProcess(errorString, tableLocation);
                bitVector[tableLocation] = pid;

                runningProcesses += 1;
                totalLaunched += 1;

                nextLaunchTimeNanos = (rand() % (500000 - 100000 + 1)) + 100000;
                currentTimeNanos = getTimeInNanoseconds();
                nextLaunchTimeNanos = currentTimeNanos + nextLaunchTimeNanos;
            }
        }


        currentTimeNanos = getTimeInNanoseconds();
        if (runningProcesses > 0 && currentTimeNanos > 100000) {

            if (msgrcv(msqID, &mesq, sizeof(mesq), getpid(), MSG_NOERROR) == -1) {
                snprintf(errorArr, 200, "\n%s Failed to recieve message to OSS.\n", errorString);
                perror(errorArr);
                exit(1);
            }


            requestCount += 1;

            /*If the process is terminating*/
            if (mesq.mesq_terminate == 1) {

                resetFrame(frameTable);

                pid = wait(&statusCode);

                if (pid != -1) {
                    printf("%s: process %d exited with status code %d\n", executable, pid, statusCode);
                    runningProcesses -= 1;
                }

                /*Add time  to clock*/
                sem_wait(&(semaphore->mutex));
                addTime(100);
                sem_post(&(semaphore->mutex));
            }

            /*Assume we we have page fault*/
            pageFault = true;

            /*if we found page table reference*/
            if (pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference] != -1) {
                printf("%s: Process %ld  has address %d in frame %d\n",
                       executable, mesq.mesq_pid, mesq.mesq_memoryAddress,
                       pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference]);


                pageFault = false;

                memoryReferenced(frameTable, executable);
            }


            /*If we detect a page fault*/
            if (pageFault) {

                if (mesq.mesq_requestType == READ) {
                    printf("%s: Process %ld is requesting read of address %d\n", executable, mesq.mesq_pid,
                           mesq.mesq_memoryAddress);
                    printf("%s: Address %d is not in a frame, pagefault\n",
                           executable, mesq.mesq_memoryAddress);
                } else {
                    printf("%s: Process %ld is requesting WRITE of address %d\n", executable, mesq.mesq_pid,
                           mesq.mesq_memoryAddress);
                    printf("%s: Address %d is not in a frame, pagefault\n",
                           executable, mesq.mesq_memoryAddress);
                }

                pageFaultCount += 1;

                pageFaultOccured(frameTable, executable);

            }


            mesq.mesq_type = mesq.mesq_pid;

            if (msgsnd(msqID, &mesq, sizeof(mesq), IPC_NOWAIT) == -1) {
                snprintf(errorArr, 200, "\n%s Failed to send message to child.\n", errorString);
                perror(errorArr);
                exit(1);
            }
        }



        /*Signal semaphore to access clock and add time to it*/
        sem_wait(&(semaphore->mutex));
        addTime(100);
        sem_post(&(semaphore->mutex));

    }


    printf("oss program ending total page faults %d total launched %d\n", pageFaultCount, totalLaunched);



    /*detach from shared memory*/
    shmdt(memoryClock);
    shmdt(semaphore);

    /*Clear shared memory*/
    shmctl(clocKID, IPC_RMID, NULL);
    shmctl(semaphoreID, IPC_RMID, NULL);
    msgctl(msqID, IPC_RMID, NULL);
    return 0;
}

void displayTable(Frame frameTable[MEM_SIZE]) {
    printf("\n\tCurrent memory layout at time %ld:%ld\n", memoryClock->seconds, memoryClock->nanoseconds);

    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        if (frameTable[i].occupiedBit == 1) {
            printf("+ ");
        } else {
            printf(". ");
        }
    }

    printf("\n");

}

void pageFaultOccured(Frame frameTable[MEM_SIZE], char *executable) {
    bool emptyFrame = false;
    int temp = 0;
    int victimFrame = 0;

    /*Find location to place page in memory*/
    int i;
    for (i = 0; i < MEM_SIZE; i++) {

        if (frameTable[i].occupiedBit == 0) {
            printf("%s: Placing process %ld page %d into frame %d\n",
                   executable, mesq.mesq_pid, mesq.mesq_pageReference, i);

            frameTable[i].pid = mesq.mesq_pid;
            frameTable[i].occupiedBit = 1;
            frameTable[i].dirtyBit = 0;
            frameTable[i].referenceByte = 1;
            frameTable[i].processPageReference = mesq.mesq_pageReference;

            /*Place frame in queue*/
            enqueue(frameQueue, i);

            /*Store frame index to processes page*/
            pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference] = i;

            if (mesq.mesq_requestType == READ) {
                frameTable[victimFrame].dirtyBit = 0;
            } else {
                frameTable[victimFrame].dirtyBit = 1;
                printf("%s: Indicating to %ld that WRITE has happened to address %d\n",
                       executable, mesq.mesq_pid, mesq.mesq_memoryAddress);
            }

            emptyFrame = true;
            break;
        }
    }


    /*If an empty frame was not found*/
    if (!emptyFrame && !isEmpty(frameQueue)) {


        /*victimFrame = dequeue(frameQueue);*/
        victimFrame = secondChanceAlgorithm(frameTable);


        printf("%s: %ld clearing frame %d and swapping in process %ld page %d\n",
               executable, mesq.mesq_pid, victimFrame, mesq.mesq_pid, mesq.mesq_pageReference);

        if (frameTable[victimFrame].dirtyBit == 1) {
            printf("%s: Dirty bit of frame %d set, adding additional time to the clock\n",
                   executable, victimFrame);

            sem_wait(&(semaphore->mutex));
            addTime(100);
            sem_post(&(semaphore->mutex));
        }

        for (i = 0; i < MAX_CONCURRENT_PROCS; i++) {
            if (bitVector[i] == frameTable[victimFrame].pid) {
                temp = i;
                break;
            }
        }

        pcbTable[temp].pageTable[frameTable[victimFrame].processPageReference] = victimFrame;


        /*update dirtybit info*/
        if (mesq.mesq_requestType == READ) {
            frameTable[victimFrame].dirtyBit = 0;
        } else {
            frameTable[victimFrame].dirtyBit = 1;
            printf("%s: Indicating to %ld that WRITE has happened to address %d\n",
                   executable, mesq.mesq_pid, mesq.mesq_memoryAddress);
        }


        frameTable[victimFrame].pid = mesq.mesq_pid;
        frameTable[victimFrame].occupiedBit = 1;
        frameTable[victimFrame].processPageReference = mesq.mesq_pageReference;
        frameTable[victimFrame].referenceByte = 1;
    }
}

int secondChanceAlgorithm(Frame frameTable[MEM_SIZE]) {
    int victim = 0;

    if(secondChanceFramePlace >= MEM_SIZE){
        secondChanceFramePlace = 0;
    }

    while(secondChanceFramePlace < MEM_SIZE){
        /*See if the second chance bit is zero
 *          * if so that is the victim frame*/
        if (second_chance[secondChanceFramePlace] == 0) {
            victim = secondChanceFramePlace;
            secondChanceFramePlace += 1;
            break;
        }
        if(second_chance[secondChanceFramePlace] == 1){
            second_chance[secondChanceFramePlace] = 0;
        }

        secondChanceFramePlace += 1;
    }

    printf("\n\nvictim %d\n\n", secondChanceFramePlace);
    return victim;
}

void memoryReferenced(Frame frameTable[MEM_SIZE], char *executable) {

    /*indicate frame reference occured*/
    frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference]].referenceByte = 1;

    if (mesq.mesq_requestType == READ) {
        printf("%s: %ld is requesting READ of address %d at time %ld:%ld\n",
               executable, mesq.mesq_pid, mesq.mesq_memoryAddress, mesq.mesq_sentSeconds, mesq.mesq_sentNS);


        /*If the dirty bit is not set take less time*/
        printf("%s: Address %d found in frame %d, giving data to process %ld at %ld:%ld\n",
               executable, mesq.mesq_memoryAddress,
               pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference], mesq.mesq_pid,
               memoryClock->seconds, memoryClock->nanoseconds);

        sem_wait(&(semaphore->mutex));
        addTime(10);
        sem_post(&(semaphore->mutex));

        /*frame was referenced so set the second chance bit to 1*/
        second_chance[pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference]] = 1;

    }

    if (mesq.mesq_requestType == WRITE) {

        printf("%s: %ld is requesting WRITE of address %d at time %ld:%ld\n",
               executable, mesq.mesq_pid, mesq.mesq_memoryAddress,
               mesq.mesq_sentSeconds, mesq.mesq_sentNS);

        printf("%s: Address %d in frame %d, WRITING data to frame at time %ld:%ld\n",
               executable, mesq.mesq_memoryAddress, pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference],
               memoryClock->seconds, memoryClock->nanoseconds);

        printf("%s: Indicating to %ld that WRITE has happened to address %d\n",
               executable, mesq.mesq_pid, mesq.mesq_memoryAddress);

        frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference]].dirtyBit = 1;

        /*frame was referenced so set the second chance bit to 1*/
        second_chance[pcbTable[mesq.mesq_pctLocation].pageTable[mesq.mesq_pageReference]] = 1;
    }

    sem_wait(&(semaphore->mutex));
    addTime(10);
    sem_post(&(semaphore->mutex));

}

void resetFrame(Frame frameTable[MEM_SIZE]) {

    /*clear bit vector location*/
    bitVector[mesq.mesq_pctLocation] = 0;

    int i;
    for (i = 0; i < PT_SIZE; i++) {
        if (pcbTable[mesq.mesq_pctLocation].pageTable[i] != -1) {

            /*Reset the frame table for this process*/
            frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[i]].pid = 0;
            frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[i]].occupiedBit = 0;
            frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[i]].dirtyBit = 0;
            frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[i]].referenceByte = 0;
            frameTable[pcbTable[mesq.mesq_pctLocation].pageTable[i]].processPageReference = -1;

            /*Reset this processes page table*/
            pcbTable[mesq.mesq_pctLocation].pageTable[i] = -1;
        }
    }
}


/*Function to connect to shared memory and error check*/
MemoryClock *connectToClock(char *error) {
    char errorArr[200];

    /*Get shared memory Id using shmget*/
    clocKID = shmget(CLOCK_KEY, sizeof(memoryClock), 0777 | IPC_CREAT);

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

/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {
    char errorArr[200];

    if ((msqID = msgget(MSG_KEY, 0777 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s Pid: %d Failed to connect to message queue", error, getpid());
        perror(errorArr);
        exit(1);
    }
}

void connectSemaphore(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((semaphoreID = shmget(SEM_KEY, sizeof(Semaphore), 0644 | IPC_CREAT)) == -1) {
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


/*Function to launch child processes and return the corresponding pid's*/
pid_t launchProcess(char *error, int openTableLocation) {
    char errorArr[200];
    pid_t pid;

    char tableLocationString[20];
    char dirtybitString[2];

    snprintf(dirtybitString, 2, "%d", dirtyBit);
    snprintf(tableLocationString, 20, "%d", openTableLocation);


    /*Fork process and capture pid*/
    pid = fork();

    if (pid == -1) {
        snprintf(errorArr, 200, "\n\n%s Fork execution failure", error);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /*executes files, Name of file to be execute, list of args following,
 *  *          * must be terminated with null*/
        execl("./userProcess", "userProcess", tableLocationString, dirtybitString, NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);

    } else {
        initializePCB(openTableLocation, pid);
        usleep(5000);
        return pid;
    }

}

void initializePCB(int location, int pid) {
    pcbTable[location].pid = pid;

    int i = 0;
    for (i = 0; i < PT_SIZE; i++) {
        pcbTable[location].pageTable[i] = -1;
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
 *  *  * It changes rear and size*/
void enqueue(Queue *queue, int item) {
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}

/*Function to remove an item from queue.
 *  *  * It changes front and size*/
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
 *  *  * of program stack enviroment*/
void ctrlCHandler() {
    char errorArr[200];
    snprintf(errorArr, 200, "\n\nCTRL+C signal caught, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(ctrlCjmp, 1);
}

/*Function handles timer alarm signal and jumps
 *  *  * to top of program stack enviroment*/
void timerHandler() {
    char errorArr[200];
    snprintf(errorArr, 200, "\n\ntimer interrupt triggered, killing all processes and releasing shared memory.");
    perror(errorArr);
    longjmp(timerjmp, 1);
}

/*Return time in nanoseconds*/
long getTimeInNanoseconds() {
    long secondsToNanoseconds = 0;
    long currentTimeInNanos;
    secondsToNanoseconds = memoryClock->seconds * 1000000000;
    currentTimeInNanos = secondsToNanoseconds + memoryClock->nanoseconds;

    return currentTimeInNanos;
}

void addTime(long nanoseconds) {
    nanoseconds += memoryClock->nanoseconds;
    if (nanoseconds > 1000000000) {
        memoryClock->seconds += 1;
        nanoseconds -= 1000000000;
        memoryClock->nanoseconds = nanoseconds;
    } else {
        memoryClock->nanoseconds += nanoseconds;
    }
}

int findTableLocation() {
    int i;

    for (i = 0; i < MAX_PROCS; i++) {
        if (bitVector[i] == 0) {
            return i;
        }
    }

    return -1;
}

void helpMessage(char *executable) {
    fprintf(stderr, "\nHelp Option Selected [-h]: The following is the proper format for executing the file:\n\n%s",
            executable);
    fprintf(stderr, " [-h] [-v]\n\n");
    fprintf(stderr, "By default just typing %s will give you information on deadlock management\n\n", executable);
    fprintf(stderr, "[-h] : Print a help message and exit.\n"
                    "[-v] : This option will be used to enable a verbose mode. In verbose mode you will get information\n"
                    "\ton the resource management as well as deadlock management\n");

    exit(EXIT_SUCCESS);

}
