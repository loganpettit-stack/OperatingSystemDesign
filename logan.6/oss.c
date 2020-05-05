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
Queue *processQueue;


int clocKID;
int semaphoreID;
int msqID;
int bitVector[MAX_PROCS] = {0};
int totalLaunched = 0;
int runningProcesses = 0;


MemoryClock *connectToClock(char *);

void connectSemaphore(char *);

void connectToMessageQueue(char *);

void ctrlCHandler();

void timerHandler();

pid_t launchProcess(char *, int);

int isEmpty(Queue *);

Queue *createQueue(unsigned int);

void enqueue(Queue *, int);

unsigned int startTimer(int);

void addTime(long);

long getTimeInNanoseconds();

int findTableLocation();

void helpMessage(char *);

int main(int argc, char *argv[]) {

    char *exe = strdup(argv[0]);
    char *executable = strdup(argv[0]);
    char *errorString = strcat(exe, ": Error: ");
    int statusCode = 0;
    int tableLocation = 0;
    FILE *outFile;
    int seconds = 5;
    char *fileName = "output.dat";
    char c;
    int dirtyBit;
    long nextLaunchTimeNanos = 0;
    long currentTimeNanos = 0;

    ProcessInfo pcbTable[MAX_CONCURRENT_PROCS];
    pid_t pid = 0;

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


        fclose(outFile);
        shmdt(memoryClock);
        shmdt(semaphore);
        shmctl(clocKID, IPC_RMID, NULL);
        shmctl(semaphoreID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }


    /* Checking command line options */
    while ((c = getopt(argc, argv, "m")) != -1)
        switch (c) {
            case 'v':
                dirtyBit = atoi(optarg);
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

    startTimer(seconds);


    /*Calculate launch time for child process between
 *      * 1 and 500 milliseconds 1 mili is 1,000,000 nanos*/
    nextLaunchTimeNanos = (rand() % (500000000 - 1000000 + 1)) + 1000000;
    currentTimeNanos = getTimeInNanoseconds();
    nextLaunchTimeNanos = currentTimeNanos + nextLaunchTimeNanos;


    while (true) {

        /*Terminate program if we have no running processes and
 *          * we have ran 100 processes*/
        if (totalLaunched >= 100 && runningProcesses == 0) {
            fprintf(stderr,
                    "\n\n%s: 100 processes have been ran and there are no longer anymore running processes, "
                    "program terminating\n", executable);
            break;
        }

        currentTimeNanos = getTimeInNanoseconds();
        /*Check to see if we are ready to launch */
        if (currentTimeNanos >= nextLaunchTimeNanos && runningProcesses < 18 && totalLaunched < 100) {

            /*Find an open location in the bit vector and launch*/
            if ((tableLocation = findTableLocation(bitVector)) != -1) {
                pid = launchProcess(errorString, tableLocation);
                runningProcesses += 1;
                totalLaunched += 1;
                printf("Launching process %d\n", pid);
            }
        }

        pid = wait(&statusCode);

        if(pid != -1) {
            printf("oss: process %d exited with status code %d\n", pid, statusCode);
            runningProcesses -= 1;
        }

        /*Signal semaphore to access clock and add time to it*/
        sem_wait(&(semaphore->mutex));

        addTime(10000);

        sem_post(&(semaphore->mutex));

    }


    /*detach from shared memory*/
    shmdt(memoryClock);
    shmdt(semaphore);

    /*Clear shared memory*/
    shmctl(clocKID,IPC_RMID, NULL);
    shmctl(semaphoreID,IPC_RMID, NULL);
    msgctl(msqID,IPC_RMID, NULL);
    return 0;
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
        execl("./userProcess", "userProcess", tableLocationString, NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);

    } else {
        usleep(5000);
        return pid;
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

    /* printf("%d enqueued to queue\n", pcbTable[item].pid);*/
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
