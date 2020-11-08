/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

void signal_handler(int signo);
void signal_handler2(int signo);
int count = 0;
int total_exec_time =0;//used by child

int pids[10];
int time_quantum[5];
int total_CPU_burst_time=0;

struct proc_t {
          int pid;
          int state;
          int remaining_tq;
          int remaining_wait;
};
struct procq_t {
          struct proc_t *p;
          struct procq_t *next;
};
struct procq_t* runq = NULL;


int main()
{
	total_CPU_burst_time =0;
	int fd[2],fd1[2],fd2[2],fd3[2],fd4[2],fd5[2],fd6[2],fd7[2],fd8[2],fd9[2];
	int burst_time=0;
	srand(time(NULL));
	for (int i = 0 ; i < 5; i++) {
		pids[i] = 0;
		time_quantum[i] = 0;
	}
	// child fork
	for (int i = 0 ; i < 10; i++) {
		
		pipe(fd);
		 int burst =0;
		pipe(fd1);
		 int burst1 =0;
		pipe(fd2);
		 int burst2 =0;
		pipe(fd3);
		 int burst3 =0;
		pipe(fd4);
		 int burst4 =0;
		pipe(fd5); 
		 int burst5 =0;
		pipe(fd6);
		 int burst6 =0;
		pipe(fd7);
		 int burst7 =0;
		pipe(fd8);
		 int burst8 =0;
		pipe(fd9);
		 int burst9 =0;

		int ret = fork();
		if (ret < 0) {
			// fork fail
			perror("fork_failed");
		} else if (ret == 0) {
			// child
			close(fd[0]);
			close(fd1[0]);
			close(fd2[0]);
			close(fd3[0]);
			close(fd4[0]);
			close(fd5[0]);
			close(fd6[0]);
			close(fd7[0]);
			close(fd8[0]);
			close(fd9[0]);
			// signal handler setup
			struct sigaction old_sa;
			struct sigaction new_sa;
			memset(&new_sa, 0, sizeof(new_sa));
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGALRM, &new_sa, &old_sa);
			//excution time
			if(i==0){
				total_exec_time = 5;
				burst = total_exec_time;
			       	write(fd[1],&burst,sizeof(burst));}
			else if(i==1){
				total_exec_time = 3;
				 burst1 = total_exec_time;
	   		         write(fd1[1],&burst1,sizeof(burst1));}
			else if(i==2){
				total_exec_time = 3;
                                burst2 = total_exec_time;
			       	write(fd2[1],&burst2,sizeof(burst2));}
			else if(i==3){
				total_exec_time = 4;
                                burst3 = total_exec_time;
			       	write(fd3[1],&burst3,sizeof(burst3));}
			else if(i==4){
				total_exec_time = 6;
                                burst4 = total_exec_time;
			       	write(fd4[1],&burst4,sizeof(burst4));}
			else if(i==5){
				total_exec_time = 8;
                                burst5 = total_exec_time;
			       	write(fd5[1],&burst5,sizeof(burst5));}
			else if(i==6){
				total_exec_time = 4;
                                burst6 = total_exec_time;
			       	write(fd6[1],&burst6,sizeof(burst6));}
			else if(i==7){
				total_exec_time = 7;
                                burst7 = total_exec_time;
			       	write(fd7[1],&burst7,sizeof(burst7));}
			else if(i==8){
				total_exec_time = 2;
                                burst8 = total_exec_time;
				write(fd8[1],&burst8,sizeof(burst8));}
			else if(i==9){
				total_exec_time = 4;
                                burst9 = total_exec_time;
			       	write(fd9[1],&burst9,sizeof(burst9));}
			while (1);
			exit(0);
			// never reach here
		} else if (ret > 0) {
			// parent
			//
			close(fd[1]);
                        close(fd1[1]);
                        close(fd2[1]);
                        close(fd3[1]);
                        close(fd4[1]);
                        close(fd5[1]);
                        close(fd6[1]);
                        close(fd7[1]);
                        close(fd8[1]);
                        close(fd9[1]);

			if(i==0) {
	 		read(fd[0], &burst,sizeof(burst));
			total_CPU_burst_time = total_CPU_burst_time + burst;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst); }
			else if(i==1) {
			read(fd1[0], &burst1,sizeof(burst1));
			total_CPU_burst_time = total_CPU_burst_time + burst1;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst1); }
			else if(i==2) {
			read(fd2[0], &burst2,sizeof(burst2));
			total_CPU_burst_time = total_CPU_burst_time + burst2;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst2); }
			else if(i==3) {
			read(fd3[0], &burst3,sizeof(burst3));
			total_CPU_burst_time = total_CPU_burst_time + burst3;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst3); }
			else if(i==4) {
			read(fd4[0], &burst4,sizeof(burst4));
			total_CPU_burst_time = total_CPU_burst_time + burst4;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst4); }
			else if(i==5) {
			read(fd5[0], &burst5,sizeof(burst5));
			total_CPU_burst_time = total_CPU_burst_time + burst5;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst5); }
			else if(i==6) {
			read(fd6[0], &burst6,sizeof(burst6));
			total_CPU_burst_time = total_CPU_burst_time + burst6;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst6); }
			else if(i==7) {
			read(fd7[0], &burst7,sizeof(burst7));
			total_CPU_burst_time = total_CPU_burst_time + burst7;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst7); }
			else if(i==8) {
			read(fd8[0], &burst8,sizeof(burst8));
			total_CPU_burst_time = total_CPU_burst_time + burst8;
			pids[i] = ret;
                        printf("child %d created, %d\n", pids[i],burst8); }
			else if(i==9){
			read(fd9[0], &burst9,sizeof(burst9));
			total_CPU_burst_time = total_CPU_burst_time + burst9;
			pids[i] = ret;
			printf("child %d created, %d\n", pids[i],burst9);}	
		}
	}

	printf("total cpu burst time is %d\n",total_CPU_burst_time);

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
	count++;
	if(count ==total_exec_time){
		printf("execution completed\n");
		exit(0);}
}

void signal_handler(int signo)
{
	int target_pid = pids[count % 10];
	 for(int i=0; i<3; i++){
	printf("(%d)->(%d) signal! count: %d \n", getpid(), target_pid, count);
	// send child a signal SIGUSR1
	//for(int i=0; i<3; i++){
	kill(target_pid, SIGALRM);
	count++;
	}

	if (count == total_CPU_burst_time) exit(0);
}

