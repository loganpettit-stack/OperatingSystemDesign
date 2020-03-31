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
#include <sys/msg.h>
#include "structures.h"

#define PCBKEY 0x1596
#define MESSAGEKEY 0x1470
#define CLOCKKEY 0x3574

/*Structure Id's*/
int pcbID;
int msqID;
int clocKID;

/*pointers to shared memory
 *  * structures*/
MessageQ msq;
MemoryClock *memoryClock;
ProcessCtrlBlk *pcbTable;

ProcessCtrlBlk *connectPcb(int, char *);

MemoryClock *connectToClock(char *);

void connectToMessageQueue(char *);

int main(int argc, char *argv[]){

    int totalPCBs = 18;
    char *executable = strdup(argv[0]);
    char *errorString = strcat(executable, ": Error: ");
    char errorArr[200];
    int tableLocation = atoi(argv[1]);
    int running = 1;
    int quantum;
    int job;
    int upper = 1;
    int lower = 0;
    srand(getpid());

    /*Connect to pcbtable in shared memory*/
    pcbTable = connectPcb(totalPCBs, errorString);


    /*Connect clock to shared memory*/
    connectToClock(errorString);

    /*Connect message queue*/
    connectToMessageQueue(errorString);


    /*printf("Launched Process pid: %d\n ppid: %d\n gid: %d\n uid: %d\n",
 *             pcbTable[tableLocation].pid, pcbTable[tableLocation].ppid, pcbTable[tableLocation].gId, pcbTable[tableLocation].uId);*/

    /*Indicate user process is ready to launch*/
    pcbTable[tableLocation].readyState = 1;

    /*removes message from queue specified by msqId, request message for this pid, generate
 *      * no_error truncates message if their is not enough room allocated*/
    if(msgrcv(msqID, &msq, sizeof(msq), getpid(), MSG_NOERROR) == -1){
        snprintf(errorArr, 200, "\n\n%s Failed to recieve message", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    printf("process %d is running at time %ld:%ld\n", getpid(), memoryClock->seconds, memoryClock->nanoseconds);



    while(running == 1){


        /*Get quantum from parent process*/
        quantum = atoi(msq.mesq_text);

        /*Generate random number between 0 and 100 to add probability
 *          * that a process will terminate*/
        srand(getpid() * memoryClock->nanoseconds);

        int terminationProbability = (rand() % 100  + 1) ;
        printf("termination probability: %d\n", terminationProbability);

        /*If the termination probability is greater than 30
 *          * do not terminate */
        if(terminationProbability > 30) {

            int quantumChoice = (rand() % 2);
            printf("quantum choice: %d\n", quantumChoice);

            /*If quantum choice is 0 it uses part of the quantum, if
 *              * it is 1 then use the full quantum*/
            if (quantumChoice == 0) {
                pcbTable[tableLocation].burstTime = (rand() % quantum);
                pcbTable[tableLocation].quantum = quantum;

                /*Choose a random job between 2 and 3*/
                job = (rand() % (3 - 2 + 1)) + 2;
            } else {
                pcbTable[tableLocation].burstTime = quantum;
                pcbTable[tableLocation].quantum = quantum;
                job = 1;
            }
        }
        else {
            /*process is terminating*/
            job = 0;
        }

        printf("\njob choice: %d\n", job);

        switch(job){

            /*job where process just terminates*/
            case 0:

                /*Give process some amount of burst time since it takes time to decided
 *                  * about termination, give it a random time within its quantum */

                pcbTable[tableLocation].quantum = quantum;
                pcbTable[tableLocation].burstTime = (rand() % pcbTable[tableLocation].quantum);
                pcbTable[tableLocation].jobFinished = 1;
                pcbTable[tableLocation].jobType = 0;

                msq.mesq_type = getppid();

                snprintf(msq.mesq_text, sizeof(msq.mesq_text),
                        "process PID %d used %d of time quantum %d and terminated",
                        pcbTable[tableLocation].pid,  pcbTable[tableLocation].burstTime, pcbTable[tableLocation].quantum);



                if(msgsnd(msqID, &msq, sizeof(msq), IPC_NOWAIT) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to send message", errorString);
                    perror(errorArr);
                    exit(1);
                }

                running = 0;
                break;


                /*Job where the process runs for full of the time quantum*/
            case 1:

                pcbTable[tableLocation].jobFinished = 0;
                pcbTable[tableLocation].jobType = 1;

                msq.mesq_type = getppid();
                snprintf(msq.mesq_text, sizeof(msq.mesq_text),
                         "process with PID %d used full time quantum of %d\n", pcbTable[tableLocation].pid, quantum);

                if(msgsnd(msqID, &msq, sizeof(msq), IPC_NOWAIT) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to send message", errorString);
                    perror(errorArr);
                    exit(1);
                }

                /*runs for full time quantum and waits for task from oss*/
                if(msgrcv(msqID, &msq, sizeof(msq), getpid(), MSG_NOERROR) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to receive message", errorString);
                    perror(errorArr);
                    exit(1);
                }

                running = 1;
                break;

                /*Job where process runs for a portion of the time quantum*/
            case 2:

                pcbTable[tableLocation].jobFinished = 0;
                pcbTable[tableLocation].jobType = 2;

                msq.mesq_type = getppid();
                snprintf(msq.mesq_text, sizeof(msq.mesq_text),
                         "used %d of %d time quantum" , pcbTable[tableLocation].burstTime, quantum);

                if(msgsnd(msqID, &msq, sizeof(msq), IPC_NOWAIT) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to send message", errorString);
                    perror(errorArr);
                    exit(1);
                }


                /*runs for a random portion of time quantum and waits for new
 *                  * task from oss*/
                if(msgrcv(msqID, &msq, sizeof(msq), getpid(), MSG_NOERROR) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to receive message", errorString);
                    perror(errorArr);
                    exit(1);
                }

                running = 1;
                break;

            case 3:

                pcbTable[tableLocation].jobFinished = 0;
                pcbTable[tableLocation].jobType = 3;

                msq.mesq_type = getppid();
                snprintf(msq.mesq_text, sizeof(msq.mesq_text), "ran %d of time provided quantum %d and was blocked",
                        pcbTable[tableLocation].burstTime, quantum);

                if(msgsnd(msqID, &msq, sizeof(msq), IPC_NOWAIT) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to send message", errorString);
                    perror(errorArr);
                    exit(1);
                }

                /*runs for a random portion of time quantum and becomes blocked, waits
 *                  * for oss to move it out of blocked queue and requeue*/
                if(msgrcv(msqID, &msq, sizeof(msq), getpid(), MSG_NOERROR) == -1){
                    snprintf(errorArr, 200, "\n%s User failed to receive message", errorString);
                    perror(errorArr);
                    exit(1);
                }

        }



     /*   msq.mesq_type = getppid();
 *           snprintf(msq.mesq_text, sizeof(msq.mesq_text), "used time quantum and terminated");
 *                   printf("Quantum: %d\n", quantum);
 *
 *                           if(msgsnd(msqID, &msq, sizeof(msq), IPC_NOWAIT) == -1){
 *                                       snprintf(errorArr, 200, "\n\n%s PID: %d Failed to send message", errorString, getpid());
 *                                                   perror(errorArr);
 *                                                               exit(EXIT_FAILURE);
 *                                                                       }
 *                                                                               running = 0; */

        if(pcbTable[tableLocation].jobFinished == 1){
            break;
        }
    }




    /*detach pcb shared memory*/
    if(shmdt(pcbTable) == -1){
        snprintf(errorArr, 200, "\n\n%s Failed to detach from proccess control blocks shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    if (shmdt(memoryClock) == -1) {
        snprintf(errorArr, 200, "\n\n%s Failed to detach the virtual clock from shared memory", errorString);
        perror(errorArr);
        exit(EXIT_FAILURE);
    }


    printf("user process %d terminating\n", getpid());
    exit(0);
}

/*Connect to a portion of shared memory for the process control blocks, create enough
 *  * space to hold 18 process control blocks*/
ProcessCtrlBlk *connectPcb(int totalpcbs, char *error) {
    char errorArr[200];

    pcbID = shmget(PCBKEY, totalpcbs * sizeof(ProcessCtrlBlk), 0777 | IPC_CREAT);
    if (pcbID == -1) {
        snprintf(errorArr, 200, "\n\n%s child PID: %d Failed to create proccess control block space", errorArr, getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    pcbTable = shmat(pcbID, NULL, 0);
    if (pcbTable == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s child PID: %d Failed to attach to process control block ", errorArr, getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return pcbTable;
}

/*Function to connect to shared memory and error check*/
MemoryClock *connectToClock(char *error) {
    char errorArr[200];
    long *sharedMemoryPtr;

    /*Get shared memory Id using shmget*/
    clocKID = shmget(CLOCKKEY, sizeof(memoryClock), 0777 | IPC_CREAT);

    /*Error check shmget*/
    if (clocKID == -1) {
        snprintf(errorArr, 100, "\n\n%s PID: %d Failed to create clock shared memory ", error, getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }


    /*Attach to shared memory using shmat*/
    memoryClock = shmat(clocKID, NULL, 0);


    /*Error check shmat*/
    if (memoryClock == (void *) -1) {
        snprintf(errorArr, 200, "\n\n%s PID: %d Failed to attach to clock shared memory ", error, getpid());
        perror(errorArr);
        exit(EXIT_FAILURE);
    }

    return memoryClock;
}


/*Function to connect the message queue to shared memory*/
void connectToMessageQueue(char *error) {

    if ((msqID = msgget(MESSAGEKEY, 0666 | IPC_CREAT)) == -1) {
        perror("Error: OSS msg queue connection failed");
        exit(1);
    }
}


