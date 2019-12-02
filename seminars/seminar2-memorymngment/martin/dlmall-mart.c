//dlmalloc.c

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0

#define HEAD (sizeof(struct head))
#define MIN(size) (((size)>(8))?(size):(8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory) ((struct head*)memory - 1)
#define HIDE(block) (void*)((struct head*)block + 1)

#define ALIGN 8
#define ARENA (64*1024)

//uint16_t: unsigned int with 16 bits of memory ie 2 bytes (unsigned short), t stands for type

struct head{
    uint16_t bfree; //2 bytes, the status of block before
    uint16_t bsize; //2 bytes, the size of block before
    uint16_t free; //2 bytes, the status of the block
    uint16_t size; //2 bytes, the size (max 2¹⁶, 64 KiB)
    struct head *next; // 8 bytes pointer
    struct head *prev; // 8 bytes pointer
};

/*Returns a pointer to the block after*/
struct head *after(struct head *block){
    /*Current poiner, cast it to a char pointer, add the size of the block plus the size of a header*/
    return (struct head*)((char*)block + block->size + HEAD);
}

/*Returns a pointer to the block before*/
struct head *before(struct head *block){
    return (struct head*)((char*)block - block->bsize - HEAD);
}

struct head *split(struct head *block, int size){
    int rsize = block->size - size - HEAD;
    block->size = rsize;

    struct head *splt = after(block);
    splt->bsize = block->size;
    splt->bfree = block->free;
    splt->size = size;
    splt->free = FALSE;

    struct head *aft = after(splt);
    aft->bsize = splt->size;
    
    return splt;
}

struct head *arena = NULL;

struct head *new(){
    if(arena != NULL){
        printf("one arena already allocated\n");
        return NULL;
    }

    /*using mmap, but could have used sbrk*/
    struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(new == MAP_FAILED){
        printf("mmap failed: error %d\n", errno);
        return NULL;
    }

    /*make room for head and dummy*/
    unsigned int size = ARENA - 2*HEAD;

    new->bfree = FALSE; //changed from NULL to FALSE (bool cant be NULL)
    new->bsize = 0; //changed from NULL to 0 (int cant be NULL)
    new->free = TRUE;
    new->size = size;

    struct head *sentinel = after(new);

    /*only touch the status fields*/
    sentinel->bfree = new->free;
    sentinel->bsize = size;
    sentinel->free = FALSE;
    sentinel->size = 0; //changed from NULL to 0 (int cant be NULL)

    /*this is the only arena we have*/
    arena = (struct head*)new;
    return new;
}

/*Global variable for the freelist*/
struct head *flist;

/*Initialises the arena/freelist*/
void init(){
    flist = (struct head*)new();
}

/*Detaches a block from the freelist*/
void detach(struct head *block){
    if(block->next != NULL){
        block->next->prev = block->prev;
    }

    if(block->prev != NULL){
        block->prev->next = block->next;
    }
    
    if(block == flist){
        flist = flist->next;
    }
}

/*Inserts a block to the front of the freelist*/
void insert(struct head *block){
    block->next = flist;
    block->prev = NULL;

    if(flist != NULL){
        flist->prev = block;
    }

    flist = block;
}

void insertInOrder(struct head *block){
    struct head* flag = flist;

    /*If freelist would be empty, insert the block as the "one and only"*/
    if(flag == NULL){
        block->next = NULL;
        block->prev = NULL;
        flist = block;
        return;
    }

    /*In order to correctly insert the block and keep the presence of the freelist, 
    we also need a flag to the block before the location where we want to insert the block*/
    struct head* previousFlag = flag->prev;

    while(flag != NULL){
        /*If block size is smaller or equal to flag size, insert it before flag*/
        if(block->size <= flag->size){
            if(previousFlag != NULL){
                previousFlag->next = block;
            }

            block->prev = previousFlag;
            flag->prev = block;
            block->next = flag;

            /*If the block to insert is the smallest one in the list, place it first*/
            if(flag == flist){
                flist = block;
            }

            return;
        }

        /*Else, continue to search for the correct location of insertion*/
        previousFlag = flag;
        flag = flag->next;
    }

    /*If block is larger than all elements in freelist, put it last in the list*/
    block->next = NULL;
    previousFlag->next = block;
    block->prev = previousFlag;
}

int adjust(size_t size){
    int rem = MIN(size) % ALIGN;
    if(rem == 0){
        return MIN(size);
    }else{
        return MIN(size) + ALIGN - rem;
    }
}

struct head* findBestFit(int size){
    struct head *i = flist;
    while(i != NULL){
        if(i->size >= LIMIT(size)){
            detach(i);
            i->free = FALSE;
            after(i)->bfree = FALSE;
            return i;
        }else{
            i = i->next;
        }
    }

    return NULL; //If we could not find a block;
}

struct head* find(int size){
    struct head *i = flist;
    while(i != NULL){
        if(i->size >= size){
            detach(i);
            if(i->size >= LIMIT(size)){
                struct head* taken = split(i, size);

                insert(before(taken));
                after(taken)->bfree = FALSE;
                taken->free = FALSE;

                return taken;
            }else{
                i->free = FALSE;
                after(i)->bfree = FALSE;

                return i;
            }

            return i;
        }else{
            i = i->next;
        }
    }

    return NULL; //If we could not find a block;
}

void *dalloc(size_t request){
    if(request <= 0){
        return NULL;
    }

    int size = adjust(request);
    struct head *taken = findBestFit(size);

    if(taken == NULL){
        return NULL;
    }else{
        return HIDE(taken);
    }
}

struct head* merge(struct head* block){
    struct head* aft = after(block);

    if(block->bfree){
        //unlink the block before
        detach(before(block));

        //calculate and set the total size of the merged blocks
        int total_size = (before(block)->size + block->size + HEAD);
        before(block)->size = total_size;

        //update the block after the merged blocks
        aft->bsize = before(block)->size;

        //continue with the merged block
        block = before(block);
    }

    if(aft->free){
        //unlink the block
        detach(aft);

        //calculate and set the total size of merged blocks
        block->size = (aft->size + block->size + HEAD);

        //update the block after the merged blocks
        aft = after(block);
        aft->bsize = block->size;        
    }

    return block;
}

void dfree(void *memory){
    if(memory != NULL){
        struct head *block = MAGIC(memory); 
        block = merge(block);
        struct head *aft = after(block);        
        block->free = TRUE;
        aft->bfree = TRUE;
        insertInOrder(block);
    }
}

/*Calculates the length of the freelist*/
int length(int numberOfAllocations){
    struct head *i = flist;
    int count = 0;
    while(i != NULL){
        count++;
        i = i->next;
    }
    printf("%d\t%d\n", numberOfAllocations, count);
}

void print(){
    struct head *i = flist;
    int count = 1;
    printf("|");
    while(i != NULL){
        printf("#%d size: %d|", count, i->size);
        count++;
        i = i->next;
    }
    printf("\n");
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