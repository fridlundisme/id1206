//test.c

#include <stdio.h>
#include <stdint.h>
#include "dlmall.h"

struct head{
    uint16_t bfree; //2 bytes, the status of block before
    uint16_t bsize; //2 bytes, the size of block before
    uint16_t free; //2 bytes, the status of the block
    uint16_t size; //2 bytes, the size (max 2¹⁶, 64 KiB)
    struct head *next; // 8 bytes pointer
    struct head *prev; // 8 bytes pointer
};

int main(){
    init();
    
    int requested = 8;
    printf("requested size for allocation: %d\n", requested);
    printf("adjusted size for allocation: %d\n", adjust(requested));
    printf("length of freelist: %d\n", length());
    print();
    printf("let's do some allocation!\n");
    void* allocated_memory = dalloc(requested);
    printf("successfully allocated memory\n");
    dfree(allocated_memory);
    printf("successfully freed memory!\n");
    int flist = (int)length();
    printf("length of freelist: %d\n", flist);
    print();

    return 0;
}