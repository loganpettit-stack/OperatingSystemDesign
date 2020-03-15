#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define SHAREDMEMKEY 0x1596
#define SEMAPHOREKEY 0x1470

typedef struct {
    int shmInt;
    sem_t mutex;
} SEMAPHORE;

SEMAPHORE *semaphore;
int semaphoreID;
int seconds = 0;

void detachSharedMemory(long *, int, char *);

long *connectToSharedMemory(int *, char *, int);

void connectSemaphore(char *);

int main(int argc, char *argv[]) {

    int childLogicalId = atoi(argv[1]);
    int startingIndex = atoi(argv[2]);
    int numbersToRead = atoi(argv[3]);
    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    long *sharedMemoryPtr;
    int sharedMemoryId;
    int originalIndex = startingIndex;
    int accumulator = 0;
    char *outfile = "adder_log";
    FILE *fptr;
    time_t t;
    time(&t);
    int upper = 3;
    int lower = 0;


    printf("\n\nprocess: %d started clock time: %s\n", childLogicalId, ctime(&t));

    /*initialize shared memory and semaphore  mutex to 1*/
    sharedMemoryPtr = connectToSharedMemory(&sharedMemoryId, errorString, numbersToRead);
    connectSemaphore(errorString);
    sem_init(&(semaphore->mutex), 1, 1);

    int x;
    for (x = 0; x < numbersToRead; x++) {

        accumulator += sharedMemoryPtr[startingIndex];
        startingIndex += 1;
    }

    sharedMemoryPtr[originalIndex] = accumulator;

    printf("%d Placing: %d into: %d\n", childLogicalId, accumulator, originalIndex);


    /*sleep for random time*/
    int randomSleepTime = (rand() % (upper - lower + 1)) + lower;
    printf("random sleep time: %d\n", randomSleepTime);
    int o;
    for (o = 0; o < randomSleepTime; o++) {
        usleep(1000000);
    }

    /*critical section,
 *      * wait a second before writing to file
 *           * wait a second before leaving sections*/
    sem_wait(&(semaphore->mutex));

    time(&t);
    fprintf(stderr, "child %d Entering critical section at: %s\n", childLogicalId, ctime(&t));

    usleep(1000000);
    fptr = fopen(outfile, "a");
    fprintf(fptr, "%-10d %-5d %-5d\n", getpid(), originalIndex, accumulator);
    usleep(1000000);

    time(&t);
    fprintf(stderr, "child %d Exiting critical section at: %s\n", childLogicalId, ctime(&t));

    sem_post(&(semaphore->mutex));

    /*Detach shared memory and semaphore*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);
    shmdt(semaphore);
    shmctl(semaphoreID, IPC_RMID, NULL);

    return 0;
}

/*Function to connect to shared memory and error check*/
long *connectToSharedMemory(int *sharedMemoryID, char *error, int numbersToRead) {
    char errorArr[200];
    long *sharedMemoryPtr;

    /*Get shared memory Id using shmget*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(int) * numbersToRead, 0777 | IPC_CREAT);

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


void connectSemaphore(char *error) {
    char errorArr[200];

    /* Getting the semaphore ID */
    if ((semaphoreID = shmget(SEMAPHOREKEY, sizeof(SEMAPHORE), 0644 | IPC_CREAT)) == -1) {
        snprintf(errorArr, 200, "\n\n%s: PID: %ld Error: Failed to allocate shared memory region for semaphore", error,
                 (long) getpid());
        perror(error);
        exit(EXIT_FAILURE);
    }

    /* Attaching to semaphore shared memory */
    if ((semaphore = (SEMAPHORE *) shmat(semaphoreID, (void *) 0, 0)) == (void *) -1) {
        snprintf(errorArr, 100, "\n\n%s: PID: %ld Error: Failed to attach to shared memory region for semaphore", error,
                 (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}


/*Function to detach from shared memory and error check*/
void detachSharedMemory(long *sharedMemoryPtr, int sharedMemoryId, char *error) {
    char errorArr[200];

    if (shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}

