#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "stdlib.h"

#include "sys/shm.h"
#include "sys/types.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "sys/wait.h"

#define SEMAPHORE_UNLOCK 1
#define SEMAPHORE_LOCK  -1

void* allocateSharedMemory(size_t memorySize, int* memID)
{
    *memID = shmget(IPC_PRIVATE, memorySize, 0600 | IPC_CREAT | IPC_EXCL);
    if(*memID <= 0)
    {
        perror("Error with shmget");
        return NULL;
    }

    void* memory = shmat(*memID, 0, 0);
    if(NULL == memory)
    {
        perror("Error with shmat");
    }
    return memory;
}

void fillArray(int *array, int arraySize, int minValue, int maxValue)
{
    srand(time(NULL));
    for(int  i = 0; i < arraySize; i++)
    {
        array[i] = minValue+ rand()%maxValue;
    }
}

void printArray( int* array, int size)
{
    for(size_t i = 0; i < size; i++)
    {
        printf("%i ", array[i]);
    }
}

void semaphoreSetStatus(int semID, int num, int status)
{
    struct sembuf op;
    op.sem_op = status;
    op.sem_flg = 0;
    op.sem_num = num;
    semop(semID, &op, 1);
}

char semaphoreLock(int semID, int num, char* semaphoreStatusArray)
{
    char key = semaphoreStatusArray[num];
    semaphoreSetStatus(semID, num, SEMAPHORE_LOCK);
    semaphoreStatusArray[num] = 1;
    return key;
}

char semaphoreLockParent(int semID, int num, char* semaphoreStatusArray)
{
    if(semaphoreStatusArray[num])
    {
        return -1;
    }
    semaphoreSetStatus(semID, num, SEMAPHORE_LOCK);
    semaphoreStatusArray[num] = 1;
    return 0;

}

void semaphoreUnlock(int semID, int num, char* semaphoreStatusArray)
{
    semaphoreSetStatus(semID, num, SEMAPHORE_UNLOCK);
    semaphoreStatusArray[num] = 0;
}

void childCode(int* array, char* semaphoreStatusArray, int size, int semID)
{
    int temp;
    for(int i = 0; i < size - 1; i++)
    {
        for(int j = 0; j < size - i - 1; j++)
        {
            semaphoreLock(semID, j, semaphoreStatusArray);
            semaphoreLock(semID, j + 1, semaphoreStatusArray);
            if(array[j] > array[j + 1])
            {
                temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
            semaphoreUnlock(semID, j, semaphoreStatusArray);
            semaphoreUnlock(semID, j + 1, semaphoreStatusArray);
        }
    }
    exit(0);
}

void parentCode(int* array, char* semaphoreStatusArray, int size, int semID, pid_t child)
{
    int iteration = 0;

    while(!waitpid(child, NULL, WNOHANG))
    {
        printf("\nIteration %i\n", iteration);
        for(int i = 0; i < size; i++)
        {
            if(semaphoreLockParent(semID, i, semaphoreStatusArray))
            {
                printf("Blocked\t");
            }
            else
            {
                printf("%d\t", array[i]);
            }
            semaphoreUnlock(semID, i, semaphoreStatusArray);
        }
        printf("\n");
        iteration++;
    }
    printf("Result: %i\n", iteration);
    printArray(array, size);
}

int main(int argv, char* argc[])
{
    if( argv <= 3)
    {
        printf("Error! Not enough params! Params required: array size, min, max");
        return -1;
    }

    int arraySize = atoi(argc[1]);
    int minValue = atoi(argc[2]);
    int maxValue = atoi(argc[3]);

    int memID;
    int* array = allocateSharedMemory(sizeof(int) * arraySize, &memID);

    fillArray(array, arraySize, minValue, maxValue);

    int semID;
    if(semID = semget(IPC_PRIVATE, arraySize, 0600 | IPC_CREAT)< 0)
    {
        perror("Error with semget()!\n");
        return -1;
    }
    printf("Semaphore set ID = %i\n", semID);

    int semaphoreStatusID;
    char* semaphoreStatusArray = allocateSharedMemory(sizeof(int) * arraySize, &semaphoreStatusID);

    for(size_t i = 0; i < arraySize; i++)
    {
        semaphoreSetStatus(semID, i, SEMAPHORE_UNLOCK);
    }

    pid_t childProcess = fork();

    if(childProcess < 0)
    {
        perror("Error with fork - process 1\n");
    }
    else if(childProcess > 0)
    {
        parentCode(array, semaphoreStatusArray, arraySize, semID, childProcess);
    }
    else
    {
        childCode(array, semaphoreStatusArray, arraySize, semID);
    }

    char freeMemoryCommand[124];

    sprintf(freeMemoryCommand, "ipcrm -m %i", memID);
	system(freeMemoryCommand);

    sprintf(freeMemoryCommand, "ipcrm -m %i", semaphoreStatusID);
	system(freeMemoryCommand);

	sprintf(freeMemoryCommand, "ipcrm -s %i", semID);
	system(freeMemoryCommand);

    return 0;
}