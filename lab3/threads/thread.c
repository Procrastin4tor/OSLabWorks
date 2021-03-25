#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* printMessageThreads()
{
    for( int i=0; i < 10; i++)
    {
        printf("Hello Threads %i \n", i),sleep(1);
    }
}

void* printmMessageIteration()
{
    for( int i=0; i < 12; i++)
    {
        printf("This is iteration %i \n", i),sleep(2);
    }
}

int main()
{
     pthread_t thread1, thread2;
     int res1 = pthread_create(&thread1, NULL, printMessageThreads, NULL);
     int res2 = pthread_create(&thread2, NULL, printmMessageIteration, NULL);
     int iret1, iret2;
     pthread_join(thread1, (void **)&iret1);
     pthread_join(thread2, (void **)&iret2); 
      
     return 0;
}
