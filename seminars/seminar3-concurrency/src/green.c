#define _POSIX_SOURCE

#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define FALSE 0
#define TRUE 1
#define PERIOD 100
#define STACK_SIZE 4096


static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, FALSE};
static green_t *running = &main_green;
static green_t* first = NULL;
static green_t* last = NULL;

static sigset_t block;

void timer_handler(int); 
static void init() __attribute__((constructor));


/* Add thread to the end of the queue */
void enqueue(green_t *thread){
    sigprocmask(SIG_BLOCK,&block,NULL);
    if(first == NULL){
        first = last = thread;
    }else{
        last->next = thread;
        last = thread;
    }
    sigprocmask(SIG_UNBLOCK,&block,NULL);
}

/* Dequeues the first thread in the ready queue */
green_t *dequeue(){
    sigprocmask(SIG_BLOCK,&block,NULL);
    if(first == NULL){
        return NULL;
    }else{
        green_t *thread = first;
        if(first == last){
            last = NULL;
        }
        first = first->next;
        thread->next = NULL;
        return thread;
    }
    sigprocmask(SIG_UNBLOCK,&block,NULL);
}


void timer_handler(int sig){
    sigprocmask(SIG_BLOCK,&block,NULL);

    green_t *susp = running;

    // add the running to the ready queue
    enqueue(susp);
    // find the next thread for execution
    green_t *next = dequeue();
    running = next;


    swapcontext(susp->context, next->context);
    sigprocmask(SIG_UNBLOCK,&block,NULL);
    
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

#pragma region green

void green_thread() {
    sigprocmask(SIG_BLOCK,&block,NULL);

    green_t *this = running;
  
    void *result = (*this->fun)(this->arg);

    // place waiting (joining) thread in ready queue
    if(this->join != NULL){
        enqueue(this->join);
    }
    // save result of execution
    this->retval = result;

    free(this->context->uc_stack.ss_sp);
    free(this->context);

    // we're a zombie
    this->zombie = TRUE;

    // find the next thread to run
    green_t *next = dequeue();
    running = next;

    setcontext(next->context);
    
    sigprocmask(SIG_UNBLOCK,&block,NULL);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {  

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

    enqueue(new);
    
    return 0;
}

int green_yield() {
    
    green_t * susp = running;
    // add susp to ready queue
    enqueue(susp);
    // select the next thread for execution
    green_t *next = dequeue();

    running = next;
    swapcontext(susp->context, next->context);

    return 0;
}

int green_join(green_t *thread, void **res) {
    sigprocmask(SIG_BLOCK,&block,NULL);

    if(thread->zombie){
        return 0;
    }
    green_t *susp = running;
    // add as joining thread
    thread->join = susp;
    //select the next thread for execution 
    green_t *next = dequeue();

    running = next;
    swapcontext(susp->context, next->context);


    // free context
    free(thread->context->uc_stack.ss_sp);
    free(thread->context);
    thread->context = NULL;

    sigprocmask(SIG_UNBLOCK,&block,NULL);

  return 0; 
}
#pragma endregion green

#pragma region conditional
void green_cond_init(green_cond_t *thread){
    thread->first = NULL;
    thread->last = NULL;
}

void green_cond_wait(green_cond_t *thread){
    sigprocmask(SIG_BLOCK,&block,NULL);
    // Suspend and queue it again
    green_t *susp = running;

    if(thread->first == NULL){
        thread->first = susp;
        thread->last = susp;
    }else{
        thread->last->next = susp;
        thread->last = susp;
    }

    green_t *next = dequeue();
    
    running = next;
    swapcontext(susp->context,next->context);
    sigprocmask(SIG_UNBLOCK,&block,NULL);
}

void green_cond_wait_mutex(green_cond_t *cond, green_mutex_t *mutex) {
    // block timer interrupt
    sigprocmask(SIG_BLOCK,&block,NULL);
    // suspend the running thread on condition
    green_t *susp = running;

    if(cond->first == NULL){
        cond->first = susp;
        cond->last = susp;
    }else{
        cond->last->next = susp;
        cond->last = susp;
    }

    if(mutex != NULL) {
        // release the lock if we have a mutex
        mutex->taken = FALSE;
        // move suspended thread to ready queue
        green_mutex_unlock(mutex);
    }
    // schedule the next thread
    green_t *next = dequeue();
    running = next;
    swapcontext(susp->context, next->context);

    if(mutex != NULL) {
    // try to take the lock 
        while(mutex->taken){
            susp = running;

            susp->next = mutex->susp;
            
            mutex->susp = susp;

            green_t *next = dequeue();
            running = next;
            swapcontext(susp->context,next->context);
        }
        mutex->taken=TRUE;
    }
    // unblock
    sigprocmask(SIG_UNBLOCK,&block,NULL);
}

void green_cond_signal(green_cond_t *thread){
    sigprocmask(SIG_BLOCK,&block,NULL);

    if(thread->first != NULL){
        green_t *var = thread->first;
        if(var->next != NULL){
            var->next = NULL;
        }
        enqueue(var);
        if(thread->last == thread->first){
            thread->last = NULL;
        }
        thread->first = thread->first->next;
    }
    sigprocmask(SIG_UNBLOCK,&block,NULL);
}

#pragma endregion contitional

#pragma region mutex

int green_mutex_init(green_mutex_t *mutex){
    mutex->taken=FALSE;
    mutex->susp=NULL;
}

int green_mutex_lock(green_mutex_t *mutex) {
    // block timer interrupt
    sigprocmask(SIG_BLOCK,&block,NULL);

    green_t *susp = running;
    if(mutex->taken) {
        // suspend the running thread 
        mutex->susp = susp;
        // find the next thread
        green_t *next = dequeue();
        running = next;
        swapcontext(susp->context, next->context);
    } else {
        // take the lock
        mutex->taken = TRUE;
    }
    // unblock
    sigprocmask(SIG_UNBLOCK,&block,NULL);
    return 0;
}

int green_mutex_unlock(green_mutex_t *mutex) {
    sigprocmask(SIG_BLOCK,&block,NULL);

    if(mutex->susp != NULL) {
        // move suspended thread to ready queue
        enqueue(mutex->susp);
    } else {
        // release lock
        mutex->taken=FALSE;
        mutex->susp = NULL;
    }
    // unblock
    sigprocmask(SIG_UNBLOCK,&block,NULL);

  return 0;
}

#pragma endregion mutex