#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(){

	int pid = fork();

	if(pid == 0){
		return 42;
	} else{
		int res;
		wait(&res);
		ptinf("The result was %d\n", WEXITSA
	}
	printf("That's the end %d\n", getpid());

	return 0;

}

