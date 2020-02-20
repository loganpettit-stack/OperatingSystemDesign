#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>

#define SHAREDMEMKEY 0x1596


/*Shared memory structure*/
struct sharedMemoryContainer {
    int seconds;
    long nanoseconds;
    int* childArray;
};

struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer*, int*, char*);
void detachSharedMemory(struct sharedMemoryContainer*, int, char*);

int main(int argc, char* argv[]) {

    struct sharedMemoryContainer* shMemptr;
    int maxChildren = 4;
    char* executable = strdup(argv[0]);
    char* errorString = strcat(executable, ": Error: ");
    int sharedMemoryId;
    struct sharedMemoryContainer* sharedMemoryPtr;

    int i;
    printf("Arg count %d\n", argc);
    for(i = 0; i < argc; i++){
        printf("Command line arg: %s\n", argv[i]);
    }

    /*connect to shared memory*/
    sharedMemoryPtr = connectToSharedMemory(sharedMemoryPtr, &sharedMemoryId, errorString);

    /*testing shared memory segment*/
    printf("%d:%ld \n", sharedMemoryPtr->seconds, sharedMemoryPtr->nanoseconds);

    /*
 *      * int j;
 *          for(j = 0; j < maxChildren; j++){
 *                  printf("%d", sharedMemoryPtr->childArray[j]);
 *                      }*/

    while(sharedMemoryPtr->seconds < 2){
        if(sharedMemoryPtr->nanoseconds > 1000000000){
            sharedMemoryPtr->seconds += 1;
        } else{
            sharedMemoryPtr->nanoseconds += 1000;
        }

    }


    /*Detach pointer to shared memory*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);

    printf("exiting child\n");

    return 0;
}

/*Function to connect to shared memory and error check*/
struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer* sharedMemoryPtr, int* sharedMemoryID, char* error){
    char errorArr[200];

    /*Get shared memory Id using shmget*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(struct sharedMemoryContainer), 0644 | IPC_CREAT);

    /*Error check shmget*/
    if (sharedMemoryID == (void *) -1) {
        snprintf(errorArr, 100, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Attach to shared memory using shmat*/
    sharedMemoryPtr = shmat(*sharedMemoryID, NULL, 0);

    /*Error check shmat*/
    if(sharedMemoryPtr == (void*) -1){
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return sharedMemoryPtr;
}

/*Function to detach from shared memory and error check*/
void detachSharedMemory (struct sharedMemoryContainer* sharedMemoryPtr, int sharedMemoryId, char* error){
    char errorArr[200];

    if(shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /* Clearing shared memory related to clockId */
    shmctl(sharedMemoryId, IPC_RMID, NULL);
}

