#include <stdio.h>
#include "../green.h"

volatile int flag;
green_cond_t cond;
green_mutex_t mutex;

/** Test with mutex lock. */
void* test(void* arg){
    int id = *(int*)arg;
    int loop = 1000;
    while(loop > 0){
        green_mutex_lock(&mutex);
        while(flag != id){
            green_mutex_unlock(&mutex);
            green_cond_wait_mutex(&cond, &mutex);
        }
        flag = (id+1)%2;
        green_cond_signal(&cond);
        green_mutex_unlock(&mutex);
        loop--;
    }
}

int main(){
  green_t g0, g1,g2;
  int a0 = 0;
  int a1 = 1;
  int a2 = 2;

  green_mutex_init(&mutex);
  green_cond_init(&cond);

  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0,NULL);
  green_join(&g1,NULL);
  
  printf("done\n");
  return 0;
}