#include <stdint.h>
#include <stdlib.h>

int length(int num);
void sanity();
void print_flist();
void *dalloc(size_t request);
void dfree(void *memory);
int adjust(size_t request);
int length();
void terminate();
int request();
void init();
int freeblockcount();
int sizeoffreeblocks();
int memoryUsed();