#include "dlmall.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
void main(void) {
    init();
    void* buffer[9];
    for (int i = 2; i < 11; i++) {
        buffer[i]  = dalloc(i);
    }
    /*for (int i = 1; i < 11; i += 2) {
        dfree(buffer[i]);
    }*/
    dfree(buffer[4]);
    dfree(buffer[2]);
    dfree(buffer[3]);

    sanity();
}
