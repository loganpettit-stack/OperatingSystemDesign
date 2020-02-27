#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define SHAREDMEMKEY 0x1596

long* connectToSharedMemorystruct(int *sharedMemoryID, char *error);
void detachSharedMemory(long*, int, char*);
long getCombinedTime(long, long);

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



    /*connect to shared memory*/
    sharedMemoryPtr = connectToSharedMemorystruct(&sharedMemoryId, errorString);

    /*testing initial shared memory segment*/
    beginTime = getCombinedTime(sharedMemoryPtr[0], sharedMemoryPtr[1]);
    killTime = beginTime + 1000000;

    if (n >= 2) {
        int i;
        for (i = 2; i <= sqrt(n); i++) {

            checkTime = getCombinedTime(sharedMemoryPtr[0], sharedMemoryPtr[1]);

            if (checkTime >= killTime) {
                sharedMemoryPtr[childLogicalId + 1] = -1;

                fprintf(stderr, "\nChild %d took too long to find prime\n", childLogicalId);

                detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);
                exit(-1);
            }

            /*If n is divisible by any number between
 *             2 and n/2, it is not prime*/
            if (n % i == 0) {
                flag = 0;
                break;
            }

            checkTime = getCombinedTime(sharedMemoryPtr[0], sharedMemoryPtr[1]);

            if (checkTime >= killTime) {
                sharedMemoryPtr[childLogicalId + 1] = -1;

                fprintf(stderr, "\nChild %d took too long to find prime\n", childLogicalId);

                printf("\nplacing in array: %ld", sharedMemoryPtr[childLogicalId + 1]);


                detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);
                exit(-1);
            }

        }
    } else {
        flag = 0;
    }


    if (flag == 1) {
       /* printf("%d is a prime number", n);*/
        sharedMemoryPtr[childLogicalId + 1] = n;
    }
    else {
       /* printf("%d is not a prime number", n);*/
        sharedMemoryPtr[childLogicalId + 1] = -n;
    }


    /*Detach pointer to shared memory*/
    detachSharedMemory(sharedMemoryPtr, sharedMemoryId, errorString);

    return 0;
}

long *connectToSharedMemorystruct(int *sharedMemoryID, char *error) {
    char errorArr[200];

/*Get shared memory Id using shmget allocate enough room for the clock and one array space*/
    *sharedMemoryID = shmget(SHAREDMEMKEY, sizeof(int) + sizeof(long) + sizeof(long), 0777 | IPC_CREAT);

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

long getCombinedTime(long seconds, long nanoseconds){
    long time = 0;

    if(seconds > 0){
        seconds *= 1000000000;
    }

    time = seconds + nanoseconds;
    return time;
}
