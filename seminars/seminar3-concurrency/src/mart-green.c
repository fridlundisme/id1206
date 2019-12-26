//green.c
#define _POSIX_SOURCE

#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>

#include "green.h"

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096
#define MAX 12

#define PERIOD 100

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;
static green_t* first = NULL;
static green_t* last = NULL;

static sigset_t block;

static void init() __attribute__((constructor));

void timer_handler(int);

/*Ready-Queue Methods*/
/*Adds the given thread to the end of the ready-queue*/
void enqueue(green_t* thread){
    if(first == NULL){
        first = last = thread;
    }else{
        last->next = thread;
        last = thread;
    }
}

/*Retrieves and removes the first thread from the ready-queue*/
green_t* dequeue(){
    if(first == NULL){
        return NULL;
    }else{
        green_t* thread = first;

        if(first == last){
            last = NULL;
        }

        first = first->next;
        thread->next = NULL;

        return thread;
    }
}

void timer_handler(int sig){
    green_t* suspended = running;

    //add the running to the ready queue
    enqueue(suspended);

    //find the next thread for execution
    green_t* next = dequeue();

    running = next;
    swapcontext(suspended->context, next->context);
}

void init(){
    getcontext(&main_cntx);

    sigemptyset(&block);
    sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    assert(sigaction(SIGVTALRM, &act, NULL) == 0);

    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &period, NULL);
}

void green_thread(){
    //block timer interrupts
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t* this = running;

    void* result = (*this->fun)(this->arg);

    //place waiting (joining) thread in ready queue
    if(this->join != NULL){
        enqueue(this->join);        
    }

    //save result of execution
    this->retval = result;

    //free allocated memory structures
    free(this->context->uc_stack.ss_sp);
    free(this->context);

    //we're a zombie
    this->zombie = TRUE;

    //find the next thread to run
    green_t* next = dequeue();

    running = next;
    setcontext(next->context);

    //unblock timer interrupts
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg){
    ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
    makecontext(cntx, green_thread, 0);

    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    //add new to the ready queue
    enqueue(new);
    
    return 0;
}

int green_yield(){
    //block timer interrupts
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t* suspended = running;

    //add suspended to the ready queue
    enqueue(suspended);

    //select the next thread for execution
    green_t* next = dequeue();

    running = next;
    swapcontext(suspended->context, next->context);

    //unblock timer interrupts
    sigprocmask(SIG_UNBLOCK, &block, NULL);

    return 0;
}

int green_join(green_t* thread, void** res){
    //block timer interrupts
    sigprocmask(SIG_BLOCK, &block, NULL);

    if(thread->zombie){
        //collect the result
        return 0;
    }

    green_t* suspended = running;

    //add as joining thread
    thread->join = suspended;

    //select the next thread for execution
    green_t* next = dequeue();

    running = next;
    swapcontext(suspended->context, next->context);

    //unblock timer interrupts
    sigprocmask(SIG_UNBLOCK, &block, NULL);

    return 0;
}

void green_cond_init(green_cond_t* var){
    //initialize a green condition variable
    var->first = NULL;
    var->last = NULL;
}


void green_cond_wait_mutex(green_cond_t *cond, green_mutex_t *mutex){
    //block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);

    //suspend the running thread on condition
    green_t *susp = running;

    if(cond->first == NULL){
        cond->first = susp;
        cond->last = susp;
    }else{
        cond->last->next = susp;
        cond->last = susp;
    }

    if(mutex != NULL){
        //release the lock if we have a mutex
        mutex->taken = FALSE;

        //schedule suspended threads 
        green_mutex_unlock(mutex);
    }
    //schedule the next thread
    green_t *next = dequeue();

    running = next;
    swapcontext(susp->context, next->context);

    if(mutex != NULL){
        //try to take the lock
        while(mutex->taken){
            //Bad luck, suspend
            susp = running;

            susp->next = mutex->susp;
            
            mutex->susp = susp;

            //find the next thread
            green_t *next = dequeue();

            running = next;
            swapcontext(susp->context, next->context);
        }
        //take the lock
        mutex->taken = TRUE;
    }
    //unblock
    sigprocmask(SIG_UNBLOCK, &block, NULL);

}

void green_cond_signal(green_cond_t* var){
    //block timer interrupts
    sigprocmask(SIG_BLOCK, &block, NULL);

    //move the first suspended thread to the ready queue
    if(var->first != NULL){
        green_t* temp = var->first;

        if(var->first == var->last){
            var->last = NULL;
        }

        var->first = var->first->next;
        temp->next = NULL;
        enqueue(temp);
    }
    
    //unblock timer interrupts
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_mutex_init(green_mutex_t* mutex){
    mutex->taken = FALSE;
    mutex->susp = NULL;
}

int green_mutex_lock(green_mutex_t* mutex){
    //block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);

    green_t* suspended = running;
    if(mutex->taken){
        //suspend the running thread
        suspended->next = mutex->susp;
        mutex->susp = suspended;

        //find the next thread
        green_t* next = dequeue();

        running = next;
        swapcontext(suspended->context, next->context);
    }else{
        //take the lock
        mutex->taken = TRUE;
    }

    //unblock timer interrupt
    sigprocmask(SIG_UNBLOCK, &block, NULL);

    return 0;
}

int green_mutex_unlock(green_mutex_t* mutex){
    //block timer interrupt
    sigprocmask(SIG_BLOCK, &block, NULL);

    if(mutex->susp != NULL){
        //move suspended threads to the ready queue
        enqueue(mutex->susp);
    }else{
        //release lock
        mutex->taken = FALSE;
    }

    //unblock timer interrupt
    sigprocmask(SIG_UNBLOCK, &block, NULL);

    return 0;
}