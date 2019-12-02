#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "rand.h"
#include "dlmall.h"

#define ROUNDS 1000
#define LOOP 10
#define BUFFER 150
// #define IMPROVE 1

int main() {
    clock_t c_start, c_stop;
    int round = 0;

    init();
    void *buffer[BUFFER];
    for (int i = 0; i < BUFFER; i++) {
        buffer[i] = NULL;
    }

    // printf("# dlmalloc graph creater.\n");
    // printf("# rounds = %d\n", (ROUNDS));
    // printf("# loops = %d\n", (LOOP));
    // printf("# buffer = %d\n", (BUFFER));
    printf("excec memoryUsed freeBlocks rounds avgsize\n");

    for(int j = BUFFER; j < ROUNDS; j++) {


      for(int i = 0; i < LOOP; i++) {
        ++round;
        int index = rand() % BUFFER;
        c_start = clock();
        if (buffer[index] != NULL) {
          dfree(buffer[index]);
        }
        size_t size = (size_t)request();
        int *memory;
        memory = dalloc(size);

        if (memory == NULL) {
          fprintf(stderr, "malloc failed\n");
          return(1);
        }
        buffer[index] = memory;
        //Writin to the memory so we know it exsits
        *memory = 123;
        c_stop = clock();

        {
          double avgsize = ((double)sizeoffreeblocks()/freeblockcount());
          double proc;
          proc = ((double)(1000000*(c_stop - c_start))/CLOCKS_PER_SEC);
          if(proc<9)
          printf("%.1f %d %d %d %.0f\n", proc, memoryUsed(), freeblockcount(), round, avgsize);
        }
      }
    }
     return 0;
}
