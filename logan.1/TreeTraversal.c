#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#define SIZE 50

struct queue {
    char* DirectoryNames[SIZE];
    int front;
    int rear;
};

struct node {
    char* directoryName;
    struct node* next;
};


struct node* createNode(char* dirName)
{
    struct node* newNode = malloc(sizeof(struct node));
    newNode->directoryName = dirName;
    newNode->next = NULL;
    return newNode;
}

struct queue* createQueue();
void enqueue(struct queue* q, char*);
char* dequeue(struct queue* q);
void display(struct queue* q);
int isEmpty(struct queue* q);
void printQueue(struct queue* q);


/*Function to create queue structure*/
struct queue* createQueue() {
    struct queue* q = malloc(sizeof(struct queue));
    q->front = -1;
    q->rear = -1;
    return q;
}

int isEmpty(struct queue* q) {
    if(q->rear == -1)
        return 1;
    else
        return 0;
}

/*Function to queue directory names into the queue*/
void enqueue(struct queue* q, char* dirName){
    if(q->rear == SIZE-1)
        printf("\nQueue is Full!!");
    else {
        if(q->front == -1)
            q->front = 0;
        q->rear++;
        q->DirectoryNames[q->rear] = dirName;
    }
}

/*Function to remove directory names from the queue*/
char* dequeue(struct queue* q){
    char* item;
    if(isEmpty(q)){
        printf("Queue is empty");
        item = " ";
    }
    else{
        item = q->DirectoryNames[q->front];
        q->front++;
        if(q->front > q->rear){
            printf("Resetting queue");
            q->front = q->rear = -1;
        }
    }
    return item;
}


void BFS(char* StartDirName){

    char* currentDirectory = " ";
    struct queue* q = createQueue();

    enqueue(q, StartDirName);

    while(!isEmpty(q)){
        currentDirectory = dequeue(q);

        /*test this portion by printing*/
        printf("%s",currentDirectory);

        DIR* d;

        /*Pointer for directory entry*/
        struct dirent *dirEntry;

        /*opendir returns a pointer of DIR type*/
        d = opendir(currentDirectory);

        if(d == NULL){
            printf("Could not open current directory");
            return;
        }

        while((dirEntry = readdir(d)) != NULL){
            enqueue(q, dirEntry->d_name);

            /*test*/
            printf("%s", dirEntry->d_name);
        }

        closedir(d);
    }




}
