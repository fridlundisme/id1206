#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "rand.h"
#include "dlmall.h"
#define ROUNDS 500
#define LOOP 10
#define BUFFER 150

int main(){


	srand((unsigned)time(NULL));

	void *current;


	for(int j = 1;j <ROUNDS; j++){
	void *buffer[BUFFER];
	for(int i = 0; i<BUFFER; i++){
		buffer[i] = NULL;
	}
		for(int i=0; i < LOOP ; i++){
			
			int index = rand() % BUFFER;
			if(buffer[index] != NULL){
				// printf("Memory freed: %p\n\n",buffer[index]);
				dfree(buffer[index]);
			}
			size_t size = (size_t)request();
			int *memory;
			memory = dalloc(size);
			// printf("Memory address: %p\n",memory);
			// printf("Memory size: %d | Memory address spaces: %d \n\n",(int)size, (int)size /16);
			if(memory == NULL){
				fprintf(stderr, "Malloc failed\n");
				return (1);
			}
						
			buffer[index] = memory;
			*memory = 123;
			/*
			 * Writing to the memory so we know it exists 
			 */
		}
		length(j);
		terminate();
	}
	return 0;
}



