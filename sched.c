/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

void signal_handler(int signo);
void signal_handler2(int signo);
int count = 0;


int pids[5];
int time_quantum[5];

int main()
{

	for (int i = 0 ; i < 5; i++) {
		pids[i] = 0;
		time_quantum[i] = 0;
	}
	// child fork
	for (int i = 0 ; i < 5; i++) {
		int ret = fork();
		if (ret < 0) {
			// fork fail
			perror("fork_failed");
		} else if (ret == 0) {
			// child

			// signal handler setup
			struct sigaction old_sa;
			struct sigaction new_sa;
			memset(&new_sa, 0, sizeof(new_sa));
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGALRM, &new_sa, &old_sa);

			while (1) ;
			exit(0);
			// never reach here
		} else if (ret > 0) {
			// parent
			pids[i] = ret;
			printf("child %d created\n", pids[i]);
		}
	}

	// signal handler setup
	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	// fire the alrm timer
	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 1;
	new_itimer.it_interval.tv_usec = 0;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	while (1);
	return 0;
}

void signal_handler2(int signo)
{
	printf("(%d) SIGALRM signaled!\n", getpid());
	exit(0);
}


void signal_handler(int signo)
{
	printf("(%d)->(%d) signal! count: %d \n", getpid(), pids[count], count);
	// send child a signal SIGUSR1
	kill(pids[count], SIGALRM);
	count++;

	if (count == 5) exit(0);
}

