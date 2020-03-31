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
#include <sys/msg.h>

#define PCBKEY 0x1596
#define MESSAGEKEY 0x1470
#define CLOCKKEY 0x3574


jmp_buf ctrlCjmp;
jmp_buf timerjmp;

/*Structure Id's*/
int pcbID;
int msqID;
int clocKID;

int runningProcesses = 0;
int totalProcesses = 0;

/*pointers to shared memory
 *  * structures*/
MessageQ msq;
MemoryClock *memoryClock;
ProcessCtrlBlk *pcbTable;

void connectToMessageQueue(char *);

MemoryClock *connectToClock(char *);

pid_t launchProcess(char *, int);

void ctrlCHandler();

void timerHandler();

unsigned int startTimer(int);

ProcessCtrlBlk *connectPcb(int, char *);

int findTableLocation(const int[], int);

Queue *createQueue(unsigned int);

void enqueue(Queue *, int);

int dequeue(Queue *);

void initializePCB(int);

void scheduleProcess(Queue *, Queue *, Queue *, Queue *, int[], char *);

long getTimeInNanoseconds(long, long);

void addTime(long nanoseconds);

int isEmpty(Queue *);

void dispatchProcess(Queue *, int, int);

int main(int argc, char *argv[]) {

    const unsigned int maxTimeBetweenNewProcsSecs = 0;
    const unsigned int maxTimeBetweenNewProcsNS = 100000;

    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];
    FILE *outputLog;
    char *fileName = "output.dat";
    long initializeScheduler = 500000000;
    int seconds = 6;
    int randomNano;
    long nextLaunchTime;
    int bitVector[18] = {0};
    int openTableLocation;
    int flag = 1;
    int pctLocation = 0;
    int totalPCBs = 18;
    int basequantum = 10;
    long currentTime = 0;
    Queue *queue1 = createQueue(18);
    Queue *queue2 = createQueue(18);
    Queue *queue3 = createQueue(18);
    Queue *blockedQueue = createQueue(18);


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

        /*Detach clock from shared memory*/
        if (shmdt(memoryClock) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }


        /*Detach process control block shared memory*/
        if (shmdt(pcbTable) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear shared memory*/
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }


    if (setjmp(timerjmp) == 1) {
        int process;

        while (!isEmpty(queue1)) {
            process = dequeue(queue1);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }

        /*Detach clock from shared memory*/
        if (shmdt(memoryClock) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Detach process control block shared memory*/
        if (shmdt(pcbTable) == -1) {
            snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
            perror(errorArr);
            exit(EXIT_FAILURE);
        }

        /*Clear shared memory*/
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);

        printf("msqID: %d", msqID);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }


    startTimer(seconds);

    /*Connect and create pcbtable in shared memory*/
    pcbTable = connectPcb(totalPCBs, errorString);

    /*Connect clock to shared memory*/
    connectToClock(errorString);

    /*Connect message queue*/
    connectToMessageQueue(errorString);

    memoryClock->seconds = 0;
    memoryClock->nanoseconds = 10;

    /*Get next random time a process should launch in nanoseconds*/
    srand(time(0));
    randomNano = (rand() % maxTimeBetweenNewProcsNS + 1);
    nextLaunchTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds + randomNano);


    nextLaunchTime = 0;
    /*Launch processes until 100 is reached*/
    while (flag) {


        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);

        /*Only allow a certian number of concurrent processes*/
        if (runningProcesses < 15 && currentTime > nextLaunchTime && totalProcesses < 100) {



            /*Start launching processes, find an open space in pcb table first*/
            if ((openTableLocation = findTableLocation(bitVector, totalPCBs)) != -1) {

                /*Launch process*/
                pid = launchProcess(errorString, openTableLocation);
                runningProcesses += 1;
                totalProcesses += 1;

                /*Generate next process launch time*/
                srand(time(0));
                randomNano = (rand() % maxTimeBetweenNewProcsNS + 1);
                nextLaunchTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds + randomNano);

                printf("next launch time: %ld\n", nextLaunchTime);

                /*Queue all processes into the first queue*/
                enqueue(queue1, openTableLocation);

                /*Show that a process is occupying a space in
 *                  * the bitvector which will directly relate to the pcb table*/
                bitVector[openTableLocation] = 1;

                printf("\nOSS: generating process with PID: %d, Putting into Queue 1 at time: %ld:%ld totalproccesses: %d.\n",
                       pid, memoryClock->seconds, memoryClock->nanoseconds, totalProcesses);

            }

        }

        currentTime = getTimeInNanoseconds(memoryClock->seconds, memoryClock->nanoseconds);

        printf("segfault?");
        if (currentTime > initializeScheduler) {
            scheduleProcess(queue1, queue2, queue3, blockedQueue, bitVector, errorString);
        }

        printf("total proc: %d, running proc: %d\n", totalProcesses, runningProcesses);

        if (runningProcesses == 0) {
            flag = 0;
        }

        addTime(100000);

    }


    printf("%d", totalProcesses);



    /*detach structures from shared memory*/
    if (shmdt(memoryClock) == -1) {
        snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }


    if (shmdt(pcbTable) == -1) {
        snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Clear shared memory*/
    shmctl(clocKID, IPC_RMID, NULL);
    shmctl(pcbID, IPC_RMID, NULL);

    printf("Clearning message queue");
    msgctl(msqID, IPC_RMID, NULL);

    return 0;
}

void scheduleProcess(Queue *q1, Queue *q2, Queue *q3, Queue *bq, int bitVector[], char *errorString) {
    int statusCode = 0;
    int upper = 2;
    int lower = 0;
    long quantum = 1000000;
    int location = 0;
    char message[20];
    int processToLaunch = 0;
    int pid = 0;
    char errorArr[200];
    long q1Quantum = quantum;
    long q2Quantum = quantum * 2;
    long q3Quantum = quantum * 4;
    /*Deque process to be scheduled and set the message queue
 *      * with its pid. */
    processToLaunch = dequeue(q1);
    pid = pcbTable[processToLaunch].pid;

    /*int y;
 *     for (y = 0; y < 18; y++){
 *             printf("bitvector: %d\n", bitVector[y]);
 *                 }
 *                     srand(memoryClock->nanoseconds);
 *                         int job = (rand() % (upper - lower + 1)) + lower;
 *                             pcbTable[processToLaunch].job = job;
 *
 *                                 printf("\nrandom job is: %d\n", job);*/



    printf("Starting scheduler for process: %d\n\n\n", pid);

    msq.mesq_type = (long) pid;

    /*Send time quantum to process and signal it to start doing work*/
    snprintf(message, 20, "%ld", quantum);
    strcpy(msq.mesq_text, message);
    if (msgsnd(msqID, &msq, sizeof(msq), MSG_NOERROR) == -1) {
        snprintf(errorArr, 200, "%s failed to send message in scheduler", errorString);
        perror(errorArr);
        exit(1);
    }


    /*When process sends back */
    if (msgrcv(msqID, &msq, sizeof(msq), getpid(), MSG_NOERROR) == -1) {
        snprintf(errorArr, 200, "%s failed to recieve message in scheduler", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
    printf("OSS: %s\n", msq.mesq_text);


    /*pid = wait(&statusCode);
 *     if (pid > 0) {
 *             fprintf(stderr, "Child with PID %ld exited with status 0x%x\n",
 *                             (long) pid, statusCode);
 *
 *                                 }*/

    /*if process job finished parent signals to kill the process*/
    if (pcbTable[processToLaunch].jobFinished == 1) {
        /* printf("killing process %d\n", pcbTable[processToLaunch].pid);
 *          kill(pcbTable[processToLaunch].pid, SIGKILL);*/
        pid = wait(&statusCode);
        if (pid > 0) {
            fprintf(stderr, "Child with PID %ld exited with status 0x%x\n",
                    (long) pid, statusCode);

        }
        runningProcesses -= 1;
        bitVector[processToLaunch] = 0;
    } else if (pcbTable[processToLaunch].jobType == 1) {
        printf("process not finished requeing\n");
        enqueue(q1, processToLaunch);
    } else if (pcbTable[processToLaunch].jobType == 2) {
        enqueue(q1, processToLaunch);
    } else {
        enqueue(q1, processToLaunch);
    }


}

/*void dispatchProcess(Queue* queue, int quatum, int location){
 *
 * }*/

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

/*Return time in nanoseconds*/
long getTimeInNanoseconds(long seconds, long nanoseconds) {
    long secondsToNanoseconds = 0;
    secondsToNanoseconds = seconds * 1000000000;
    nanoseconds += secondsToNanoseconds;

    return nanoseconds;
}

/*Connect to a portion of shared memory for the process control blocks, create enough
 *  * space to hold 18 process control blocks*/
ProcessCtrlBlk *connectPcb(int totalpcbs, char *error) {
    char errorArr[200];

    pcbID = shmget(PCBKEY, totalpcbs * sizeof(ProcessCtrlBlk), 0777 | IPC_CREAT);
    if (pcbID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space for process control block",
                 errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to attach to shared memory process control block", errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return pcbTable;
}


/*Function to find an open location in the bit vector which will correspond to the spot
 *  * in the pcb table*/
int findTableLocation(const int bitVector[], int totalPCBs) {
    int i;

    for (i = 0; i < totalPCBs; i++) {
        if (bitVector[i] == 0) {
            return i;
        }
    }

    return -1;
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


/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {
    char errorArr[200];

    if ((msqID = msgget(MESSAGEKEY, 0777 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s Pid: %d Failed to connect to message queue", error, getpid());
        perror(errorArr);
        exit(1);
    }

    printf("message queue connected to\n");
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
        execl("./userProcess", "userProcess", tableLocationString, NULL);

        snprintf(errorArr, 200, "\n\n%s execution of user subprogram failure ", error);
        perror(errorArr);
        exit(EXIT_FAILURE);

    } else {
        usleep(500);
        return pid;
    }

}

void initializePCB(int tableLocation) {
    pcbTable[tableLocation].pid = getpid();
    pcbTable[tableLocation].ppid = getppid();
    pcbTable[tableLocation].uId = getuid();
    pcbTable[tableLocation].gId = getgid();

    /* printf("oss: Process pid: %d\n ppid: %d\n gid: %d\n uid: %d\n\n",
 *              pcbTable[tableLocation].pid, pcbTable[tableLocation].ppid, pcbTable[tableLocation].gId,
 *                           pcbTable[tableLocation].uId);*/
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
