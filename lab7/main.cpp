#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "pthread.h"

#include "sys/ipc.h"
#include "sys/msg.h"

#include <iostream>

typedef struct{
    long mtype;
    int mdata[4];
    char isLast;
} msgStruct;

void swap(int *a, int i, int j)
{
  int s = a[i];
  a[i] = a[j];
  a[j] = s;
}

void printMsg(int *a, int n)
{
  static int num = 1; 
  std::cout.width(3); 
  std::cout << num++ << ": ";
  for (int i = 0; i < n; i++)
  {
    std::cout << a[i] << " ";  
  }
  std::cout << std::endl;
}

bool permutationsAlgorithm(int *a, int n)
{
  int j = n - 2;
  while (j != -1 && a[j] >= a[j + 1]) j--;
    if (j == -1)
        return false; 
  int k = n - 1;
  while (a[j] >= a[k]) k--;
    swap(a, j, k);
  int l = j + 1, r = n - 1; 
  while (l<r)
    swap(a, l++, r--);
  return true;
}

void* pthreadFunction(void* args)
{
    msgStruct pthreadMsg;
    int msgID = *((int*) args);

    ssize_t len = msgrcv(msgID, &pthreadMsg, sizeof(pthreadMsg), 0, 0);

    do
    {
      if(!std::next_permutation(pthreadMsg.mdata, pthreadMsg.mdata + 4))
      {
        pthreadMsg.isLast = 1;
        msgsnd(msgID, &pthreadMsg, sizeof(pthreadMsg), 0);
        break;
      }
      msgsnd(msgID, &pthreadMsg, sizeof(pthreadMsg), 0);
    } while(1);
  
    return 0;
}

int main(void)
{
    int randomNumber[4];
    srand(time(NULL));
    for(int i = 0; i < 4; i++)
    {
        randomNumber[i] = rand() % 9;
    }

    int msgID;
    if(msgID = msgget(IPC_PRIVATE, 0666 | IPC_CREAT) < 0)
    {
        perror("Error with msgget()!\n");
        return -1;
    }

    pthread_t thread;
    int result = pthread_create(&thread, NULL, pthreadFunction, &msgID);

    msgStruct parentMsg;
    parentMsg.mtype = 1;
    parentMsg.isLast = 0;
    
    for(int i = 0; i < 4; i++)
    {
        parentMsg.mdata[i] = randomNumber[i];
    }
    msgsnd(msgID, &parentMsg, sizeof(parentMsg), 0);

    while(!parentMsg.isLast)
    {
        msgrcv(msgID, &parentMsg, sizeof(parentMsg), 0, 0);
        printMsg(parentMsg.mdata, 4);
    };

    msgctl(msgID, IPC_RMID, NULL);
    return 0;
}