#include "stdlib.h"
#include "stdio.h"

int main(int argv, char* argc[])
{
    if(argv <= 2)
    {
        printf("Error! Not enough arguments!");
        return -1;
    }

    const char* filename = argc[1];
    size_t bytes = atoi(argc[2]);
    FILE* file = fopen(filename, "r");

    if(!file)
    {
        printf("Error! File cannot be opened!\n");
        return -1;
    }

    char* buffer = malloc(bytes);
    int result = fread(buffer, sizeof(char), bytes, file);

    if(!result)
    {
        printf("Error. File cannot be read!\n");
    }
    else
    {
        for(size_t i = 0; i < bytes; i++)
        {
            printf("num := %i\t symbol := %c\t code := (0x%x)\n", buffer[i], buffer[i], buffer[i]);
        }
        printf("\n");
    }

    free(buffer);
    fclose(file);

    return 0;
}