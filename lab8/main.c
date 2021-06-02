#include "stdio.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "sys/types.h"

int main(int argv, char* argc[])
{
    if(argv <= 2)
    {
        printf("Error! Not enough arguments! There are 2 arguments required: input file name, output file name\n");
        return -1;
    }

    const char* inputFilename = argc[1];
    const char* outputFilename = argc[2];

    int inputFile = open(inputFilename, O_RDONLY, 0600);

    if(inputFile < 0)
    {
        printf("Error! Couldn't create or open file!");
        return -1;
    }

    struct stat inputFileStat;

    if(fstat(inputFile, &inputFileStat) == -1)
    {
        printf("Error! Couldn't get stat");
        return -1;
    }

    int outputFile = open(outputFilename, O_RDWR | O_CREAT, 0600);

    if(outputFile < 0)
    {
        printf("Error! Couldn't create or open file!");
        return -1;
    }

    int maxlength = inputFileStat.st_size;
    ftruncate(outputFile, maxlength);

    char* mmapOutputFile = (char*) mmap(NULL, maxlength, PROT_WRITE | PROT_READ, MAP_SHARED, outputFile, 0);

    if(mmapOutputFile == MAP_FAILED)
    {
        printf("Error with mmap()! \n");
        return -1;
    }
    
    close(outputFile);

    ssize_t outputLength = read(inputFile, mmapOutputFile, maxlength);
    close(inputFile);

    printf("Input file symbols count: %i\n", maxlength);
    printf("Output file symbols count: %li\n", outputLength);

    munmap((void*)mmapOutputFile, maxlength);

    return 0;
}