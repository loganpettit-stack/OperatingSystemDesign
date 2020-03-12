#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#define SHAREDMEMKEY 0x1596


void detachSharedMemory(long *, int, char *);
long *connectToSharedMemory(int *, char *, int);


int main(int argc, char* argv[]) {

    int childLogicalId = atoi(argv[1]);
    int startingIndex = atoi(argv[2]);
    int numbersToRead = atoi(argv[3]);
    char* executable = strdup(argv[0]);
    char* errorString = strcat(executable, ": Error: ");
    long* sharedMemoryPtr;
    int sharedMemoryId;
    int originalIndex = startingIndex;
    int accumulator = 0;

    printf("Child logical id: %d\n\n", childLogicalId);
    printf("Starting index: %d\n\n", startingIndex);

    sharedMemoryPtr = connectToSharedMemory(&sharedMemoryId, errorString, numbersToRead);


    int x;
    for(x = 0; x < numbersToRead; x++){

        accumulator += sharedMemoryPtr[startingIndex];
        startingIndex += 1;
    }





    sharedMemoryPtr[originalIndex] = accumulator;
  
  /*  printf("Placing: %d into: %d\n", accumulator, originalIndex);
 *
 *      int t;
 *          for(t = 0; t < 64; t++){
 *                  printf("shared memory pointer: %ld\n", sharedMemoryPtr[t]);
 *                      }
 *                      */

    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);

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


/*Function to detach from shared memory and error check*/
void detachSharedMemory(long *sharedMemoryPtr, int sharedMemoryId, char *error) {
    char errorArr[200];

    if (shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}
