#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

struct taken {
	uint16_t bfree;
	uint16_t bsize;
	uint16_t free;
	uint16_t size;
};

struct head {
    uint16_t bfree;
    uint16_t bsize;
    uint16_t free;
    uint16_t size;	//2 bytes, the size (max size is 64 Kbytes)
    struct head *next;
    struct head *prev;
};

#define TRUE 1
#define FALSE 0
#define IMPROVE 1

#ifdef IMPROVE

#define HEAD (sizeof(struct taken))
#define MIN(size) (((size)>(16))?(size):(16))
#define MAGIC(memory) (struct head*)((struct taken*)memory - 1)
#define HIDE(block) (void*)((struct taken*)block + 1)

#else

#define HEAD (sizeof(struct head))
#define MIN(size) (((size)>(8))?(size):(8))
#define MAGIC(memory) ((struct head*)memory - 1)
#define HIDE(block) (void*)((struct head*)block + 1)

#endif

#define LIMIT(size) (MIN(0) + HEAD + size)
#define ALIGN 8
#define ARENA (64*1024)

struct head *after(struct head *block){
    return (struct head*)((char*)block + (block->size + HEAD));
}

struct head *before(struct head *block) {
    return (struct head*)((char*)block - (block->bsize + HEAD));
}

struct head *split(struct head *block, int size) {
    int rsize = block->size - size - HEAD;
    block->size = rsize;

    struct head *splt = after(block);
    splt->bsize = block->size;
    splt->bfree = block->free;
    splt->size = size;
    splt->free = FALSE;

    struct head *aft = after(splt);
    aft->bsize = splt->size;
    aft->bfree = FALSE;

    return splt;
}

struct head *arena = NULL;

struct head *new() {
    if (arena != NULL) {
        return NULL;
    }

    struct head *new = mmap(NULL, ARENA,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new == MAP_FAILED) {
        printf("mmap failed: error %d\n", errno);
        return NULL;
    }

    uint size = ARENA - 2*HEAD;

    new->bfree = FALSE;
    new->bsize = 0;
    new->free = TRUE;
    new->size = size;

    struct head *sentinel = after(new);
    sentinel->bfree = new->free;
    sentinel->bsize = new->size;
    sentinel->free = FALSE;
    sentinel->size = 0;
    arena = (struct head*)new;
    return new;
}

struct head *flist;

void detatch(struct head *block) {
    if(block->next != NULL) {
        block->next->prev = block->prev;
    }

    if (block->prev != NULL) {
        block->prev->next = block->next;
    }
    if (block == flist) {
        flist = flist->next;
    }
}

void insert(struct head *block) {
    block->next = flist;
    block->prev = NULL;
    if (flist != NULL) {
        flist->prev = block;
    }
    flist = block;
}

int adjust(size_t request) {
    request = LIMIT(request);
    if (request%ALIGN){
        return request - request%ALIGN;
    }
    return request;
}


struct head *find(int size) {
    struct head *tmp = flist;
    while(tmp != NULL) {
        if (size < tmp->size) {
            detatch(tmp);
            if (LIMIT(size) <= tmp->size) {
                struct head *splt = split(tmp, size);
                insert(before(splt));
                return splt;
            }
            tmp->free = FALSE;
            after(tmp)->bfree = FALSE;
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

void loop() {
    struct head *tmp = flist;
    int i = 0;
    int totSize = 0;
    while (tmp != NULL) {
        printf("SIZE OF ELEMENT %d\n", (tmp->size));
        totSize += (int)(tmp->size + HEAD);
        tmp = tmp->next;
        i++;
    }
    printf("NUMBER OF ELEMENTS IN FLIST: %d\n", i);
    printf("TOTAL SIZE OF ELEMENTS IN LIST: %d\n", totSize);
}

void init() {
    flist = (struct head*)new();
}

void *dalloc(size_t request) {
    //loop();
    if (request <= 0) {
        return NULL;
    }
    int size = adjust(request);
    struct head *taken = find(size);
    if (taken == NULL) {
        return NULL;
    }
    else {
        return HIDE(taken);
    }
}


struct head *merge(struct head *block) {
    struct head *aft = after(block);
    if (block->bfree) {
        detatch(before(block));

        uint totsize = block->size + before(block)->size + HEAD;
        before(block)->size = totsize;
        aft->bsize = totsize;

        block = before(block);
    }

    if (aft->free) {
        detatch(aft);

        uint totsize = block->size + aft->size + HEAD;
        block->size = totsize;
        after(aft)->bsize = totsize;
    }
    return block;
}

void dfree(void *memory) {
    if (memory != NULL){
        struct head *block = MAGIC(memory);
        block = merge(block);
        block->free = TRUE;
        after(block)->bfree = TRUE;
        insert(block);
    }
    return;
}


int length(int numberOfAllocations){
    struct head *i = flist;
    int count = 0;
    while(i != NULL){
        count++;
        i = i->next;
    }
    printf("%d\t%d\n", numberOfAllocations, count);
}



void terminate(){
    arena = NULL;
    flist = NULL;
}

void sanity(){
    printf("sanity check:\n");

    struct head* i = flist;
    int sanity = TRUE;

    if(i == NULL){
        printf("free list is empty!\n");
    }else{
        int count = 1;
        while(i != NULL){
            if(i->free == FALSE){
                printf("block #%d is not free!\n", count);
                sanity = FALSE;
            }else{
                printf("block #%d is free.\n", count);
            }

            if(i->size < MIN(0)){
                printf("block #%d is too small! (%d)\n", count, i->size);
                sanity = FALSE;
            }else{
                printf("block #%d size is correct (%d).\n", count, i->size);
            }

            if(i->size % 8 != 0){
                printf("block #%d size is not multiple of 8!\n", count);
                sanity = FALSE;
            }else{
                printf("block #%d size is multiple of 8\n", count);
            }

            if(i->next != NULL){
                if(i->next->prev != i){
                    printf("block #%d incorrect pointer to block #%d (%p -/-> %p)\n", count+1, count, i->next->prev, i);
                    sanity = FALSE;
                }else{
                    printf("block #%d correct pointer to block #%d (%p --> %p)\n", count+1, count, i->next->prev, i);
                }
            }
            count++;
            i = i->next;
        }

        if(sanity == TRUE){
            printf("sanity result: common sense\n");
        }else{
            printf("sanity result: absurd\n");
        }
    }
}
int freeblockcount(){
    int i = 0;
    struct head *tmp = flist;
    while (tmp != NULL) {
        tmp = tmp->next;
        ++i;
    }
    return i;
}
int sizeoffreeblocks() {
    int i = 0;
    struct head *tmp = flist;
    while (tmp != NULL) {
        i += tmp->size + HEAD;
        tmp = tmp->next;
    }
    return i;
}
int memoryUsed(){
    return ARENA - (ARENA - sizeoffreeblocks());
}