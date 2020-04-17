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
Queue *processq;

int clocKID;
int pcbID;
int semaphoreID;
int descriptorID;
int msqID;
int bitVector[totalPcbs] = {0};
int maxResources;
int linesWritten = 0;
int deadlockCount = 0;
int totalLaunched = 0;
int runningProcesses = 0;
int requestGrantCount = 0;
bool verbose = false;


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

void initializeResources();

unsigned int startTimer(int);

long getTimeInNanoseconds();

int findTableLocation();

void initializePCB(int);

void updateResourceDescriptor(int);

void allocateResources(int, char *, FILE *);

bool checkSafeState();

void addTime(long);

void displayMatrix(int[totalPcbs][numResources], FILE *);

int main(int argc, char *argv[]) {

    char *exe = strdup(argv[0]);
    char *executable = strdup(argv[0]);
    char *errorString = strcat(exe, ": Error: ");
    char errorArr[200];
    long nextLaunchTimeSeconds = 0;
    long nextLaunchTimeNanos = 0;
    long currentTimeNanos = 0;
    int seconds = 3;
    int c;
    int process = 0;
    int tableLocation = 0;
    FILE *outFile;
    char *fileName = "output.dat";
    bool runFlag;
    int processToRun;
    int statusCode = 0;

    processq = createQueue(18);

    outFile = fopen(fileName, "w");

    /*Able to represent process ID*/
    pid_t pid = 0;


    /*Handle abnormal termination*/
    /*Catch ctrl+c signal and handle with
 *     * ctrlChandler*/
    signal(SIGINT, ctrlCHandler);

    /*Catch alarm generated from timer
 *     * and handle with timer Handler */
    signal(SIGALRM, timerHandler);


    /*Jump back to main enviroment where Concurrentchildren are launched
 *     * but before the wait*/
    if (setjmp(ctrlCjmp) == 1) {

        snprintf(errorArr, 200, "\n\n%s: Ctrl + C interupt signal sent, terminating program ", executable);
        perror(errorArr);

        int u;
        for (u = 0; u < totalPcbs; u++) {
            if (bitVector[u] == 1) {
                kill(pcbTable[u].pid, SIGKILL);
            }
        }

        while (!isEmpty(processq)) {
            process = dequeue(processq);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }

        fclose(outFile);
        shmdt(pcbTable);
        shmdt(memoryClock);
        shmdt(semaphore);
        shmdt(resources);
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);
        shmctl(semaphoreID, IPC_RMID, NULL);
        shmctl(descriptorID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }
    if (setjmp(timerjmp) == 1) {

        snprintf(errorArr, 200, "\n\n%s: timer interupt signal sent, terminating program ", executable);
        perror(errorArr);


        int u;
        for (u = 0; u < totalPcbs; u++) {
            if (bitVector[u] == 1) {
                kill(pcbTable[u].pid, SIGKILL);
            }
        }

        while (!isEmpty(processq)) {
            process = dequeue(processq);
            pid = pcbTable[process].pid;
            kill(pid, SIGKILL);
        }

        fclose(outFile);
        shmdt(pcbTable);
        shmdt(memoryClock);
        shmdt(semaphore);
        shmdt(resources);
        shmctl(pcbID, IPC_RMID, NULL);
        shmctl(clocKID, IPC_RMID, NULL);
        shmctl(semaphoreID, IPC_RMID, NULL);
        shmctl(descriptorID, IPC_RMID, NULL);
        msgctl(msqID, IPC_RMID, NULL);

        exit(EXIT_SUCCESS);
    }


    /* Checking command line options */
    while ((c = getopt(argc, argv, "hv")) != -1)
        switch (c) {
            case 'h':
                printf("h chose\n");
                break;
            case 'v':
                verbose = true;
                break;
            default:
                verbose = false;
        }


    connectToClock(errorString);
    connectSemaphore(errorString);
    connectToMessageQueue(errorString);

    /*Get Pcb table from creating a 1D array
 *      * of process control blocks*/
    pcbTable = connectPcb(totalPcbs, errorString);

    /*Connect to resource descriptor*/
    connectDescriptor(errorString);


    /*Initialize clock*/
    memoryClock->seconds = 0;
    memoryClock->nanoseconds = 0;

    /*Initialize semaphore*/
    sem_init(&(semaphore->mutex), 1, 1);

    /*Set resource descriptor with initial values*/
    initializeResources();

    /*Calculate launch time for child process between
 *      * 1 and 500 milliseconds 1 mili is 1,000,000 nanos*/
    nextLaunchTimeNanos = (rand() % (500000000 - 1000000 + 1)) + 1000000;
    currentTimeNanos = getTimeInNanoseconds();
    nextLaunchTimeNanos = currentTimeNanos + nextLaunchTimeNanos;

    startTimer(seconds);


    while (true) {

        printf("time is: %ld:%ld\n", memoryClock->seconds, memoryClock->nanoseconds);

        /*Terminate program if we have no running processes and
 *          * we have ran 100 processes*/
        if (totalLaunched >= 100 && runningProcesses == 0) {
            fprintf(stderr,
                    "\n\n%s: 100 processes have been ran and there are no longer anymore running processes, "
                    "program terminating\n", executable);
            break;
        }

        /*Close outfile if we have written 100,000 lines*/
        if (linesWritten >= 100000) {
            fclose(outFile);
        }

        addTime(10000);

        /*Launch another process if we can*/
        currentTimeNanos = getTimeInNanoseconds();
        if (currentTimeNanos >= nextLaunchTimeNanos && runningProcesses < 18 && totalLaunched <= 100) {

            /*Find open place in pcbtable from bitvector*/
            if ((tableLocation = findTableLocation(bitVector)) != -1) {

                printf("\nLaunching process %d\n", pid);
                pid = launchProcess(errorString, tableLocation);


                bitVector[tableLocation] = 1;
                runningProcesses += 1;
                totalLaunched += 1;

                /*Get next time to launch a process*/
                nextLaunchTimeNanos = (rand() % (500000000 - 1000000 + 1)) + 1000000;
                currentTimeNanos = getTimeInNanoseconds();
                nextLaunchTimeNanos = currentTimeNanos + nextLaunchTimeNanos;
            }

        }

        usleep(500);


        addTime(10000);

        /*Enter critical section*/
        sem_wait(&(semaphore->mutex));

        printf("oss is entering crit section\n");

        if (runningProcesses > 0) {

            /*Check to see if child is trying to terminate*/
            int i;
            for (i = 0; i < totalPcbs; i++) {


                /*If process is trying to terminate de-allocate resources*/
                if (resources->terminating[i] == 1) {

                    printf("oss: process %d is trying to terminate\n", pcbTable[i].pid);

                    /* wait(&statusCode);*/
                    kill(pcbTable[i].pid, SIGKILL);

                    printf("\n%s: Child %d is terminating\n", executable, pcbTable[i].pid);

                    if (verbose) {
                        fprintf(outFile, "\n%s: Child %d is terminating", executable, pcbTable[i].pid);
                        printf("\n%s: Child %d is terminating with status code %d", executable, pcbTable[i].pid,
                               statusCode);
                    }


                    /*Reset data in resource descriptor*/
                    updateResourceDescriptor(i);

                    /*Resources reset and process has been terminated
 *                      * attempt running blocked processes*/
                    if (!isEmpty(processq)) {
                        printf("jumped into blocked processes queue size %d\n", processq->size);
                        runFlag = true;
                        processToRun = dequeue(processq);


                        /*Create matrix for what process needs*/
                        int m;
                        int k;
                        for (m = 0; m < totalPcbs; m++) {
                            for (k = 0; k < numResources; k++) {
                                resources->needMatrix[m][k] =
                                        resources->requestMatrix[m][k] - resources->allocationMatrix[m][k];
                            }
                        }

                        /*If there are not enough resources for the process
 *                          * requeue the process*/
                        int h;
                        for (h = 0; h < numResources; h++) {
                            if (resources->allocationVector[h] - resources->needMatrix[processToRun][h] < 0) {
                                enqueue(processq, processToRun);
                                runFlag = false;
                                break;
                            }
                        }

                        /*If there are enough resources for the process, run it*/
                        if (runFlag) {
                            mesq->mesq_type = pcbTable[processToRun].pid;

                            snprintf(mesq->mesq_text, sizeof(mesq->mesq_text), "Run and Terminate");
                            if (msgsnd(msqID, &mesq, sizeof(mesq), MSG_NOERROR) == -1) {
                                snprintf(errorArr, 200, "%s failed to send message", errorString);
                                perror(errorArr);
                                exit(1);
                            }

                            /*Process request granted*/
                            requestGrantCount += 1;
                            resources->suspended[processToRun] = 0;
                            resources->allocate[processToRun] = 1;
                            break;
                        }

                    }

                    printf("jumped past blocked quque\n");

                }

                addTime(10000);

            }

            /*Check if any processes are requesting resources*/
            for (i = 0; i < totalPcbs; i++) {
                /*upon resource request check if we are not
 *                  in a deadlock state and allocate*/
                printf("checking if any processes are requestning resources\n");
                if (resources->request[i] == 1) {
                    allocateResources(i, executable, outFile);
                }
            }


            printf("attempt to handle processes in queue size: %d\n", processq->size);
            /*Attempt to empty queue by terminating processes*/
            if (!isEmpty(processq)) {

                if (processq->size == runningProcesses) {
                    while (!isEmpty(processq)) {
                        processToRun = dequeue(processq);

                        /*Allow process to requests resources*/
                        resources->request[processToRun] = 1;
                        resources->allocate[processToRun] = 0;
                        resources->release[processToRun] = 0;
                        resources->terminating[processToRun] = 0;
                        resources->suspended[processToRun] = 0;

                        mesq->mesq_type = pcbTable[processToRun].pid;
                        snprintf(mesq->mesq_text, sizeof(mesq->mesq_text), "Run and Terminate");
                        if (msgsnd(msqID, &mesq, sizeof(mesq), MSG_NOERROR) == -1) {
                            snprintf(errorArr, 200, "%s failed to send message in scheduler", errorString);
                            perror(errorArr);
                            exit(1);
                        }
                    }
                }
            }
        }

        /*increment time*/
        addTime(10000);

        sem_post(&(semaphore->mutex));

    }

    /*wait(&statusCode);*/

    fprintf(stderr, "\n%s Program exiting after running 100 processes\n", executable);
    fprintf(stderr, "\n%s Total requests granted: %d\n", executable, requestGrantCount);
    fprintf(stderr, "\n%s Total deadlocks detected: %d\n", executable, deadlockCount);

    fclose(outFile);

    while (!isEmpty(processq)) {
        process = dequeue(processq);
        pid = pcbTable[process].pid;
        kill(pid, SIGKILL);
    }


    /*detach from shared memory*/
    shmdt(pcbTable);
    shmdt(memoryClock);
    shmdt(semaphore);
    shmdt(resources);

    /*Clear shared memory*/
    shmctl(clocKID, IPC_RMID, NULL);
    shmctl(pcbID, IPC_RMID, NULL);
    shmctl(semaphoreID, IPC_RMID, NULL);
    shmctl(descriptorID, IPC_RMID, NULL);
    msgctl(msqID, IPC_RMID, NULL);
    return 0;
}

void allocateResources(int location, char *executable, FILE *outFile) {
    bool suspend = false;
    bool safe;


    /*set up needs matrix*/
    int m;
    int k;
    for (m = 0; m < totalPcbs; m++) {
        for (k = 0; k < numResources; k++) {
            /*What a process needs is what it is
 *              requesting - what it has allocated*/
            resources->needMatrix[m][k] =
                    resources->requestMatrix[m][k] - resources->allocationMatrix[m][k];
        }
    }

    int i;
    for (i = 0; i < numResources; i++) {

        /*if process requests greater than claims break*/
        if ((resources->allocationMatrix[location][i] + resources->needMatrix[location][i]) >
            resources->requestMatrix[location][i]) {
            resources->release[i] = 1;
            break;
        }
            /*if process is requesting more than available suspend*/
        else if (resources->needMatrix[location][i] > resources->allocationVector[i]) {
            suspend = true;
            break;
        } else {
            resources->allocationMatrix[location][i] += resources->needMatrix[location][i];
            resources->allocationVector[i] -= resources->needMatrix[location][i];
        }
    }

    safe = checkSafeState();

    printf("safe state is: %d\n", safe);

    if (safe && !suspend) {
        fprintf(stderr, "\n%s: child %d was granted resources.\n", executable, pcbTable[location].pid);
        if (verbose) {
            fprintf(outFile, "\n%s: child %d was granted resources\n", executable, pcbTable[location].pid);
            linesWritten += 1;

            resources->request[location] = 0;
            resources->allocate[location] = 1;
            requestGrantCount += 1;

            printf("seg fault 1?");
            if (requestGrantCount % 20 == 0) {
                fprintf(outFile, "\n%s 20 resource requests have been granted, printing matrix\n", executable);
                linesWritten += 1;
                displayMatrix(resources->allocationMatrix, outFile);
            }
        }

    } else {
        deadlockCount += 1;
        if (deadlockCount % 10 == 0) {
            fprintf(stderr, "\n%s: %d deadlocks detected\n", executable, deadlockCount);
        }

        /*Unsafe state detected reset to safe state*/
        fprintf(outFile, "\n%s: Deadlock detected ", executable);
        if (verbose) {
            fprintf(outFile, " at time %ld:%ld", memoryClock->seconds, memoryClock->nanoseconds);
            linesWritten += 1;
        }
        fprintf(outFile, ".");
        linesWritten += 1;

        resources->suspended[location] = 1;
        resources->request[location] = 1;
        enqueue(processq, location);
    }


    printf("made it through allocateResources function\n");

}

void displayMatrix(int matrix[totalPcbs][numResources], FILE *outFile) {

    fprintf(outFile, "\n\t");
    int i;
    for (i = 0; i < numResources; i++) {
        fprintf(outFile, "R%02d  ", i);
    }

    fprintf(outFile, "\n\n");

    int j;
    for (i = 0; i < totalPcbs; i++) {
        fprintf(outFile, "P%d\t", i);
        for (j = 0; j < numResources; j++) {
            fprintf(outFile, "| %02d ", matrix[i][j]);
        }

        fprintf(outFile, "|\n");
    }
    fprintf(outFile, "\n");
    linesWritten += 23;
}


/***May need void as a parameter***/
/*Function checks if the os is in a safe state*/
bool checkSafeState() {

    printf("In check safe state function\n");
    int resourceQuantity[numResources] = {0};
    bool processesFinished[totalPcbs] = {0};
    int safe[totalPcbs];
    bool found;

    /*set up needs matrix*/
    int m;
    int k;
    for (m = 0; m < totalPcbs; m++) {
        for (k = 0; k < numResources; k++) {
            resources->needMatrix[m][k] =
                    resources->requestMatrix[m][k] - resources->allocationMatrix[m][k];

            printf(" %d ", resources->needMatrix[m][k]);
        }

        printf("\n");
    }


    printf("\n");

    /*set resourceQuantity to the current resource vector state*/
    printf("resource vector\n");

    int i;
    for (i = 0; i < numResources; i++) {
        resourceQuantity[i] = resources->resourceVector[i];
        printf(" %d ", resourceQuantity[i]);
    }

    /*Find a process whos needs we can grant*/
    int count = 0;
    while (count < totalPcbs) {
        found = false;

        /*Check if process is still active or finished*/
        for (i = 0; i < totalPcbs; i++) {

            if (processesFinished[i] == 0) {


                /*check if this processes resource needs is
 *                  * greater than actual resources*/
                int j;
                for (j = 0; j < numResources; j++) {
                    if (resources->needMatrix[i][j] > resourceQuantity[j]) {
                        printf("needs exceeded available resources breaking\n");
                        break;
                    }
                }

                /*if a process can request all resources
 *                  * it needs */
                if (j == numResources) {

                    printf("safe number of requests found\n");

                    for (j = 0; j < numResources; j++) {
                        resourceQuantity[j] += resources->allocationMatrix[i][j];
                    }


                    /*mark process as safe*/
                    safe[count++] = i;

                    processesFinished[i] = 1;
                    found = true;
                }
            }
        }


        /*If any process is marked as not safe*/
        if (found == false) {
            printf("no processes found safe\n");
            return false;
        }
    }

    return true;
}


/*Function to reset the resource descriptor and handle other
 *  * clean ups after process termination*/
void updateResourceDescriptor(int processLocation) {


    bitVector[processLocation] = 0;
    runningProcesses -= 1;

    resources->terminating[processLocation] = 0;
    resources->request[processLocation] = 0;
    resources->allocate[processLocation] = 0;
    resources->release[processLocation] = 0;
    resources->suspended[processLocation] = 0;


    /*Reset resources in descriptor*/
    int i;
    for (i = 0; i < numResources; i++) {

        /*Add nonshareable resources back to the
 *          * allocation vector*/
        if (resources->sharableResources[i] != 1) {
            resources->allocationVector[i] += resources->allocationMatrix[processLocation][i];
        }

        resources->allocationMatrix[processLocation][i] = 0;
        resources->requestMatrix[processLocation][i] = 0;
    }

    printf("updated resource descriptor\n");
}

void initializeResources() {
    int i;
    int j;

    srand(time(0));

    for (i = 0; i < numResources; i++) {
        /*Give each resource a random number
 *          * between 1 an 10 instances*/
        resources->resourceVector[i] = (rand() % (10 - 1 + 1)) + 1;
        printf("random Resources: %d\n", resources->resourceVector[i]);

        /*resouces available to be used*/
        resources->allocationVector[i] = resources->resourceVector[i];

        /*Create sharable resources*/
        if (i % 4 == 0) {
            resources->sharableResources[i] = 1;
        }

        resources->request[i] = 0;
        resources->allocate[i] = 0;
        resources->release[i] = 0;
        resources->terminating[i] = 0;
        resources->suspended[i] = 0;
        resources->timesChecked[i] = 0;
    }


    /*Initialize the matrices to 0*/
    for (i = 0; i < totalPcbs; i++) {
        for (j = 0; j < numResources; j++) {
            resources->requestMatrix[i][j] = 0;
            resources->allocationMatrix[i][j] = 0;
            resources->needMatrix[i][j] = 0;
        }
    }
}

/*Function to connect to shared memory and error check*/
MemoryClock *connectToClock(char *error) {
    char errorArr[200];

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
                 error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to attach to shared memory process control block",
                 error, (long) getpid());
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

void connectDescriptor(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((descriptorID = shmget(DESCRIPTORKEY, sizeof(ResourceDiscriptor), 0644 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200,
                 "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for resource descriptor", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if ((resources = (ResourceDiscriptor *) shmat(descriptorID, (void *) 0, 0)) == (void *) -1) {
        snprintf(errorArr, 200,
                 "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for resource descriptor", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}


/*Function to find an open location in the bit vector which will correspond to the spot
 *  * in the pcb table*/
int findTableLocation() {
    int i;

    for (i = 0; i < totalPcbs; i++) {
        if (bitVector[i] == 0) {
            return i;
        }
    }

    return -1;
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
        usleep(5000);
        return pid;
    }

}

void initializePCB(int tableLocation) {
    pcbTable[tableLocation].pid = getpid();
    pcbTable[tableLocation].ppid = getppid();
    pcbTable[tableLocation].uId = getuid();
    pcbTable[tableLocation].gId = getgid();

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
