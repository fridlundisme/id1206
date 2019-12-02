#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

int adjust();
int length();
struct head *merge();
// struct head *rfind();
struct head *find();
// void *find();
void *rfind();
void detach();


struct head {
    uint16_t bfree; // The status of the block before, TRUE or FALSE = free or not
    uint16_t bsize; // The size of the block before
    uint16_t free; // The status of the block, TRUE or FALSE = free or not
    uint16_t size; // The size 2^16 = 64 Ki bytes
    struct head *next;
    struct head *prev;
};

#define TRUE 1
#define FALSE 0

#define HEAD (sizeof(struct head))
#define MIN(size) (((size)>(8))?(size):(8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory) ((struct head*)memory - 1)
#define HIDE(block) (void*)((struct head*)block + 1)
#define ALIGN 8
#define ARENA (64*1024)

/*
 * Function: *after
 * -----------------
 * Calculates the address to the pointer after the argument pointer
 * 
 * *block: pointer to the block to search for
 * 
 * return: pointer to the next block
 */
struct head *after (struct head *block){
    return (struct head*)((char*) block + (HEAD + block->size));
}

/*
 * Function: *before
 * -----------------
 * Calculates the address to the pointer before the argument pointer
 * 
 * *block: pointer to the block to search for
 * 
 * return: pointer to the next block
 */
struct head *before (struct head *block){
    return (struct head*)((char*) block- block->size - HEAD);
}

/*
 * Function: *split
 * -----------------
 * 
 * *block:  pointer to the block to search for
 * size:    requested size of the new block  
 * 
 * return:  
 * 
 */
struct head *split (struct head *block, int size){
    // printf("SPLIT\n");
    int rsize = block->size-size - HEAD;
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


/*
 * Function: *new
 * --------------------------------------
 * Creates a new block of memory by allocating as large of an area as possible
 */
struct head *new(){

    if(arena != NULL){
        // printf("One arena already allocated \n");
        return NULL;
    }
    struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(new == MAP_FAILED){
        printf("mmap failed: error %d \n", errno);
        return NULL;
    }

    unsigned int size = ARENA - 2*HEAD;

    new->bfree = FALSE;
    new->bsize = 0;
    new->free = TRUE;
    new->size = size;

    struct head *sentinel = after(new);

    sentinel->bfree = TRUE;
    sentinel->bsize = size;
    sentinel->free = FALSE;
    sentinel->size = 0;

    arena = (struct head*)new;
    return new;
}
struct head *flist;

void init(){
    flist = (struct head*)new();
}


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

void insert(struct head *block){
    // printf("INSERT\n");
    block->next = flist;
    block->prev = NULL;

    if(flist != NULL){
       flist->prev = block; 
    }
    flist = block;
}


void *dalloc(size_t request){
    // printf("First row in dalloc");
    if(request <= 0){
        return NULL;
    }
    
    int size = adjust(request);
    struct head *taken = find(size);
    if(taken == NULL)
        return NULL;
    else
        return HIDE(taken);
}

void dfree(void *memory){
    struct head *tmp = memory;
    if(memory != NULL){
        struct head *block = MAGIC(memory);
        // block = merge(block);
        
        struct head *aft = after(block);
        block->free = TRUE;
        aft->bfree = TRUE;
        insert(block);
    }
    return;
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
        after(aft)->bsize = block->size;        
    }

    return block;
}


int adjust(size_t size){
    size = LIMIT(size);
    if(size%ALIGN){
        return size + ALIGN - size%ALIGN;
    }
    return size;
}


struct head *find(int size) {
    struct head *tmp = flist;
    while(tmp != NULL) {
        if (size < tmp->size) {
            detach(tmp);
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

// void *find(int size) {
//     struct head *block = flist;
//     while (block != NULL && block->size < size) {
//         block = block->next;
//     }
//     if (block == NULL) {
//         return NULL;
//     }  
//     detach(block);
//     if (LIMIT(size) <= block->size) {
//         struct head *new = split(block, size);
//         insert(block);
//         return new;
//     }else {
//         block->free = FALSE;
//         after(block)->bfree = FALSE;

//         return block;
//     }
// }


// void *find(size_t size){
//     struct head *list = flist;
//     return rfind(list, size);
// }

// void *rfind(struct head *list, size_t size){
//     if(list->size < size){
//         return rfind(list->next, size);
//     }else if (list == NULL){
//         return NULL;
//     }else{
//         detach(list);
//         if(LIMIT(size) <= list->size){
//             struct head *tmp = split(list,size);
//             insert(list);
//             return HIDE(tmp);
//         }else{
//         list->free = FALSE;
//         after(list)->bfree = FALSE;
//         }
//     }

// }



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

void printFList(){

}

void print_flist() {
  int list_len = 0;
  struct head *walk = flist;  
  while (walk->size != 0) {
    printf("address: %p, free: %d, size: %d\n", walk, walk->free, walk->size);
    if(walk->free)
        list_len++;
    walk = after(walk);
  }
  printf("The flist is %d long\n", list_len);
}
