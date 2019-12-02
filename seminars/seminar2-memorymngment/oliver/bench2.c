//bench.c

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include "dlmall.h"

#define BUFFER 100
#define MAX 100
#define MIN 8


/*Makes memory allocations*/
void doAllocations(int amount){
    /*Declaring and initializing buffer of pointers to allocated data segments*/
    void* buffer[BUFFER];
    for(int i = 0; i < BUFFER; i++){
        buffer[i] = NULL;
    }

    /*Freeing data segments*/
    for(int i = 0; i < amount; i++){
        int index = rand() % BUFFER;
        if(buffer[index] != NULL){
            dfree(buffer[index]);
        }

        size_t size = (size_t)request(MAX);
        int* memory = dalloc(size);

        /*Check if allocation was successful*/
        if(memory == NULL){
            fprintf(stderr, "memory allocation failed!\n");
            return;
        }

        /*Allocation was successful: add pointer to buffer*/
        buffer[index] = memory;
        /*Write something to the data segment (memory) so we know it exists*/
        *memory = 123;
    }
}

/*Benchmarks the length of the freelist*/
void benchmarkFLLength(int amount){
    printf("Benchmarking length of freelist\n");
    for(int i = BUFFER; i < amount; i += 10){
        init();
        doAllocations(i);
        length(i);
    }
}

int main(int argc, const char *argv[]){
    if(argc < 2){
        printf("Please enter the number of allocations that should be made!\n");
        exit(1);
    }

    /*The amount of allocations that should be made during the benchmark*/
    int amount = atoi(argv[1]);

    benchmarkFLLength(amount);

    return 0;
}

