#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int volatile done;

void handler(int sig, siginfo_t *siginfo, void *context){
	printf("signal %d was caught\n", sig);

	printf("Your UID is %d\n", siginfo->si_uid);
	printf("Your PID is %d\n", siginfo->si_pid);

	done = 1;
}

int main(){
	struct sigaction sa;

	int pid = getpid();

	printf("Ok, let's go, kill me (%d) if you can!\n", pid);

	// We're using the more elaborated sigaction handler
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;

	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGINT, &sa, NULL) != 0){
		return(1);
	}

	while(!done){
	}

	printf("Told you so!\n");
	return(0);
}
