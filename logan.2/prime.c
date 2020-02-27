#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define SHAREDMEMKEY 0x1596


/*Shared memory structure
 * struct sharedMemoryContainer {
 *     int seconds;
 *         long nanoseconds;
 *             int* childArray;
 *             };
 *             */
/*struct sharedMemoryContainer* connectToSharedMemory(struct sharedMemoryContainer*, int*, char*);*/
/*void detachSharedMemory(struct sharedMemoryContainer*, int, char*);*/
long* connectToSharedMemorystruct(int *sharedMemoryID, char *error);
void detachSharedMemory(long*, int, char*);
long getCombinedTime(int, long);

int main(int argc, char* argv[]) {

    /* struct sharedMemoryContainer *sharedMemoryPtr;*/
    int childLogicalId = atoi(argv[1]);
    char* executable = strdup(argv[0]);
    int n = atoi(argv[2]);
    int flag = 1;
    char* errorString = strcat(executable, ": Error: ");
    int sharedMemoryId;
    long* sharedMemoryPtr;
    long beginTime = 0;
    long killTime = 0;
    long checkTime = 0;



    /*connect to shared memory
 *     sharedMemoryPtr = connectToSharedMemory(sharedMemoryPtr, &sharedMemoryId, errorString);*/
    sharedMemoryPtr = connectToSharedMemorystruct(&sharedMemoryId, errorString);

    /*testing initial shared memory segment
 *     printf("\nChild %d entered at: %ld:%ld \n", childLogicalId, sharedMemoryPtr[0], sharedMemoryPtr[1]);*/
    beginTime = getCombinedTime(sharedMemoryPtr[0], sharedMemoryPtr[1]);
    killTime = beginTime + 1000000;


    int i;
    for (i = 2; i <= sqrt(n); i++) {

        checkTime = getCombinedTime(sharedMemoryPtr[0], sharedMemoryPtr[1]);
        if(checkTime >= killTime){
            sharedMemoryPtr[childLogicalId + 2] = -1;

            printf("Child took too long to find prime");
            detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);

        }

        /*If n is divisible by any number between
 *         2 and n/2, it is not prime*/
        if (n % i == 0) {
            flag = 0;
            break;
        }
    }

    if (flag == 1) {
       /* printf("%d is a prime number", n);*/
        sharedMemoryPtr[childLogicalId + 1] = n;
    }
    else {
       /* printf("%d is not a prime number", n);*/
        sharedMemoryPtr[childLogicalId + 1] = -n;
    }

   /* printf("child looking at array: \n");
 *     int k;
 *         for (k = 0; k < childLogicalId; k++) {
 *                 printf("%d", sharedMemoryPtr[k]);
 *                     }*
 *                         printf("\nExiting child %d at: %ld:%ld\n\n", childLogicalId, sharedMemoryPtr[0], sharedMemoryPtr[1]);*/

    /*Detach pointer to shared memory*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);

    return 0;
}

long *connectToSharedMemorystruct(int *sharedMemoryID, char *error) {
    char errorArr[200];

/*Get shared memory Id using shmget allocate enough room for the clock and one array space*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(int) + sizeof(long) + sizeof(long), 0644 | IPC_CREAT);

/*Error check shmget*/
    if (sharedMemoryID == (void *) -1) {
        snprintf(errorArr, 100, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }


    long* sharedMemoryPtr;

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
void detachSharedMemory (long* sharedMemoryPtr, int sharedMemoryId, char* error){
    char errorArr[200];

    if(shmdt(sharedMemoryPtr) == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory ", error, (long) getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }
}

long getCombinedTime(int seconds, long nanoseconds){
    long time = 0;

    if(seconds > 0){
        seconds *= 1000000000;
    }

    time = seconds + nanoseconds;
    return time;
}
