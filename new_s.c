/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
//#define t_quantum 3
#include "msg.h"
typedef struct p_PCB{
	int pid;
	int state;
	int burst_time;
	int burst_time2;
	int remaining_wait;
	int time_quantum;
	struct p_PCB *next;
	int again;
}p_PCB;

typedef struct Run_q{
	p_PCB* front;
	p_PCB* rear;
	int count;
}Run_q;

void signal_handler(int signo);
void signal_handler2(int signo);
void add_queue(struct Run_q* run_q,int pid,int burst, int again);
void InitQueue(struct Run_q *run_q);
int IsEmpty(struct Run_q* run_q);
struct p_PCB* pop_queue(struct Run_q* run_q);
int count = 0;
int count2= 0;
int wait = 0;
int total_exec_time =0;//used by child
int first_exec_time = 0;
int second_exec_time = 0;
int pids[10];
int time_quantum[5];
int total_CPU_burst_time=0;

struct Run_q run_q;
struct Run_q wait_q;
//struct p_PCB* PCB[10];
//struct p_PCB r_PCB;

int main()
{
	int fd[2];
	srand(time(NULL));
	InitQueue(&run_q);
	InitQueue(&wait_q);
	for (int i = 0 ; i < 10; i++) {
		pids[i] = 0;
		time_quantum[i] = rand()%10+1;
//		time_quantum[i] = (20-i)+2;
	}
	// child fork
	for (int i = 0 ; i < 10; i++) {	
		pipe(fd);
		int burst =0;
		int ret = fork();
		if (ret < 0) {
			// fork fail
			perror("fork_failed");
		} else if (ret == 0) {
			// child
			close(fd[0]);
			// signal handler setup
			struct sigaction old_sa;
			struct sigaction new_sa;
			memset(&new_sa, 0, sizeof(new_sa));
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGALRM, &new_sa, &old_sa);
			//excution time
			burst = time_quantum[i];
			first_exec_time = burst;
			//second_exec_time = time_quantum[i+10];

			write(fd[1],&burst,sizeof(burst));
			while (1);
			exit(0);
			// never reach here
		} else if (ret > 0) {
			// parent
			close(fd[1]);
			read(fd[0], &burst,sizeof(burst));
			total_CPU_burst_time = total_CPU_burst_time + burst;
			pids[i] =ret;

			//InitQueue(run_q);
			add_queue(&run_q,pids[i],burst, 0);
			printf("child %d created, %d\n", pids[i],burst);
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
	printf("---handler2 start---\n");
	printf("  H2: (%d) SIGALRM signaled!\n", getpid());	
	if(wait !=0){
                wait -= 1;
                printf("  H2: WAITQUE_%d, REMAINING_TIME_%d\n", getpid(), wait);
	}
	else{
		count++;
		//count가 first_time보다 작으면
		if(count<first_exec_time){
			printf("  H2: first exec\n");
			//보통
		}
		//count가 first_time이면
		else if(count==first_exec_time){
			//wait queue 진입
			printf("  H2: wait queue\n");
			wait = 5;
			second_exec_time = 7;
		}
		//count가 first_time보다 크면
		else if(count<first_exec_time+second_exec_time){
			//보통
			printf("  H2: second exec\n");
		}
		//count가 first_time+second_time이면
		else if (count == first_exec_time + second_exec_time){	
	                printf("  H2: (%d) execution completed@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n",getpid());
			printf("---handler2 end---\n");
        	        exit(0);
		}
	}
	printf("---handler2 end---\n");
}

void signal_handler(int signo)
{
	printf("---handler1 start---\n");
	if(!IsEmpty(&wait_q)){
		struct p_PCB* w_PCB = malloc(sizeof(p_PCB));
		w_PCB = wait_q.front;
		int target_pid2 = w_PCB->pid;
		printf("   H1: WAIT(%d): %d\n", target_pid2, w_PCB->remaining_wait);
		kill(target_pid2, SIGALRM);
		w_PCB->remaining_wait -= 1;
		if(w_PCB->remaining_wait == 0){
			w_PCB = pop_queue(&wait_q);
			w_PCB->burst_time = 7;
			add_queue(&run_q, w_PCB->pid, w_PCB->burst_time, 1);
		}

	}
	if(!IsEmpty(&run_q)){
		struct p_PCB* r_PCB = malloc(sizeof(p_PCB));
		r_PCB = run_q.front;
		int target_pid = r_PCB->pid;
		printf("   H1: (%d)->(%d) signal! count: %d \n", getpid(), target_pid, count);
		// send child a signal SIGUSR1
		kill(target_pid, SIGALRM);
		count++;
		r_PCB->burst_time -= 1;

		if((r_PCB->burst_time ==0)&(r_PCB->again == 1)){
			r_PCB = pop_queue(&run_q);
			free(r_PCB);
		}
		else if ((r_PCB->burst_time ==0)&(r_PCB->again == 0)){
			//wait queue 진입
			r_PCB = pop_queue(&run_q);
                        add_queue(&wait_q,r_PCB->pid, r_PCB->burst_time, 0);
                        r_PCB->remaining_wait = 5;
		}
		else if(r_PCB->time_quantum == 0){
			r_PCB = pop_queue(&run_q);
			add_queue(&run_q,r_PCB->pid, r_PCB->burst_time, r_PCB->again);
		}
		else{
			r_PCB->time_quantum -= 1;
		}
	}
	if (count == total_CPU_burst_time){
		printf("   H1: finished!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");	
		exit(0);}
	printf("---handler1 end---\n");
}

void add_queue(struct Run_q* run_q,int pid, int burst, int again) {

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode->pid = pid;
	newNode->burst_time = burst;
	newNode->state= 0;
	newNode->remaining_wait = 5;
	newNode->next = NULL;
	newNode->time_quantum = 2;
	newNode->again = again;
	if (IsEmpty(run_q)) {
		run_q->front = run_q->rear = newNode;
	}
	else {
		run_q->rear->next  = newNode;
		run_q->rear =newNode;
	}

	run_q->count++;
}

struct p_PCB* pop_queue(struct Run_q* run_q){

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode = run_q->front;
	run_q->front  = newNode->next;
	run_q->count--;	
	return newNode;	
}
void InitQueue(struct Run_q *run_q){

	run_q->front = run_q->rear = NULL;
	run_q->count =0;
}
int IsEmpty(struct Run_q* run_q){

	if(run_q->count ==0)
		return 1;
	else
		return 0;
}
