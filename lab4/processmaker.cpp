#include <stdio.h> 
#include <sys/shm.h> 
#include <stdlib.h> 
#include <unistd.h>

#include <sys/types.h> 
#include <sys/wait.h>
#include "time.h"

void *allocateSharedMemory(size_t memSize, int &memId)
{
    memId = shmget(IPC_PRIVATE, memSize, 0600|IPC_CREAT|IPC_EXCL);
    if (memId <= 0)     
    {         
        perror("error with memId");         
        return NULL;     
    }

    void *mem = shmat(memId, 0, 0);
    
    if (NULL == mem)     
    {         
        perror("error with shmat");     
    }          
    
    return mem; 
}

void printArray(int* arr)
{
   for (int i = 0; i < 20; i++)
      printf("%i ", *(arr + i));
   printf("\n");
}

int compareValue(const void* a, const void* b)
{
   return *((int*) a) - *((int*) b);
}

void childMainCode(int* sharedMem)
{
   qsort(sharedMem, 20, 4, compareValue);
   printArray(sharedMem);
   exit(0);
}

int main() {                
    int memId;     
    int *sharedMem = (int *)allocateSharedMemory(1024, memId);      
    printf("memId = %d\n", memId);      
    printf("starting child process...\n"); 

    srand(time(NULL));
    for(int i = 0; i < 20; i++)
        *(sharedMem + i) = rand()%100;
    
    printArray(sharedMem);

    pid_t childId = fork();     
   
    if (childId < 0)     
    {         
        perror("error with fork()\n");     
    }     
    else if (childId > 0)     
    {         
                
        waitpid(childId, NULL, 0);              
    }     
    else     
    {         
        childMainCode(sharedMem);         
    } 
            
    char sharedMemDel[124];
    sprintf(sharedMemDel, "ipcrm -m %i", memId);
    system(sharedMemDel);   

    return 0; 
    }