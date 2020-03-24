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

#define SHAREDMEMKEY 0x1596

/*Structure Id's*/
int pcbID;
int msqID;
int clocKID;

/*pointers to shared memory
 *  * structures*/
MessageQ *msq;
MemoryClock *memoryClock;
ProcessCtrlBlk *pcbTable;

ProcessCtrlBlk *connectPcb(int, char *);


int main(int argc, char *argv[]){

    int totalPCBs = 18;
    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];
    int tableLocation = atoi(argv[1]);

    /*Connect to pcbtable in shared memory*/
    pcbTable = connectPcb(totalPCBs, errorString);


    printf("Launched Process pid: %d\n ppid: %d\n gid: %d\n uid: %d\n",
            pcbTable[tableLocation].pid, pcbTable[tableLocation].ppid, pcbTable[tableLocation].gId, pcbTable[tableLocation].uId);


    /*detach pcb shared memory*/
    if(shmdt(pcbTable) == -1){
        snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    /*Clear pcb shared memory*/
    shmctl(pcbID, IPC_RMID, NULL);

    return 0;
}

/*Connect to a portion of shared memory for the process control blocks, create enough
 *  * space to hold 18 process control blocks*/
ProcessCtrlBlk *connectPcb(int totalpcbs, char *error) {
    char errorArr[200];

    pcbID = shmget(SHAREDMEMKEY, totalpcbs * sizeof(ProcessCtrlBlk), 0777 | IPC_CREAT);
    if (pcbID == -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to create shared memory space", errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %ld Failed to attach to shared memory ", errorArr);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return pcbTable;
}


