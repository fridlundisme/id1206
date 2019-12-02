#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define PAGES (4096)
#define PAGESIZE 64
#define REFS (1048*1024)

int main(int argc, char *argv[]){
    clock_t c_start, c_stop;

    char *memory = malloc ((long)PAGESIZE * PAGES);

    for(int p = 0; p < PAGES; p++){
        long ref = (long)p * PAGESIZE;
        memory[ref] +=1;
    }

        printf("#TLB experiment\n");
        printf("#  page size = %d bytes\n", (PAGESIZE));
        printf("#  max number of pages = %d\n",  PAGES);
        printf("#  total number or references = %d Mi\n", (REFS/(1024*1024)));
        printf("#pages\t proc\t wall\t sum\n");  

    for(int pages=1; pages <= PAGES; pages+=1){
        int loops = REFS /pages;

        c_start = clock();

        long sum = 0;

        for(int l=0; l<loops;l++){
            for(int p =0; p<pages; p++){
                long ref = (long)p * PAGESIZE;
                sum+= memory[ref];
            }
        }


        
        c_stop = clock();

        {
            double proc;
            proc = ((double) (c_stop - c_start))/CLOCKS_PER_SEC;

            printf("%d\t %.4f\t %ld\n", pages, proc, sum);
        
        }
    }
    return 0;
}