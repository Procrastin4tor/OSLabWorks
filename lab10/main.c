#include "stdio.h"
#include "stdlib.h"
#include "signal.h"

int sigInt = 0, sigUsr1 = 0, sigUsr2 = 0;

void handler(int nsig)
{
    switch(nsig)
    {
        case SIGINT: sigInt++;
            break;
        case SIGUSR1: sigUsr1++;
            break;
        case SIGUSR2: sigUsr2++;
            break;
    }
}

int main(void)
{
    signal(SIGINT, handler);
    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);
    
    kill(0, SIGUSR1);
    kill(0, SIGUSR1);
    kill(0, SIGUSR1);

    while(1)
    {
        printf("\n");
        system("date");

        if(sigInt < 5)
        {
            printf("SIGINT: [%d]\n", sigInt);
            printf("SIGUSR1: [%d]\n", sigUsr1);
            printf("SIGUSR2: [%d]\n", sigUsr2);
        }
        else
        { 
            kill(0, SIGUSR2);

            printf("Result:\n");
            printf("SIGINT: [%d]\n", sigInt);
            printf("SIGUSR1: [%d]\n", sigUsr1);
            printf("SIGUSR2: [%d]\n", sigUsr2);
            break;
        }
        sleep(1);
    }

    return 0;
}