/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "msg.h"
#define t_quantum 5

//messge
int msgq;
int ret;
int key = 0x23456;
struct msgbuf msg;

typedef struct p_PCB{
	int pid;
	int state;
	int burst_time;
	int remaining_wait;
	int time_quantum;
	struct p_PCB *next;
}p_PCB;

typedef struct Run_q{
	p_PCB* front;
	p_PCB* rear;
	int count;
}Run_q;

void signal_handler(int signo);
void signal_handler2(int signo);
void signal_handler3(int signo);
void add_queue(struct Run_q* run_q,int pid,int burst);
void InitQueue(struct Run_q *run_q);
int IsEmpty(struct Run_q* run_q);
struct p_PCB* pop_queue(struct Run_q* run_q);
int count = 0;
int total_exec_time =0;//used by child
int pids[10];
int time_quantum[10];
int total_CPU_burst_time=0;
int parent_pid=0;

struct Run_q run_q;
struct Run_q wait_q;

int main()
{
	int fd[2];
	srand(time(NULL));
	parent_pid = getpid();
	InitQueue(&run_q);
	InitQueue(&wait_q);

	for (int i = 0 ; i < 10; i++) {
		pids[i] = 0;
		time_quantum[i] = rand()%20+7;
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
			total_exec_time = burst;
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

			add_queue(&run_q,pids[i],burst);
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

	// I/O signal handler setup
	struct sigaction old_sa2;
	struct sigaction new_sa2;
	memset(&new_sa, 0, sizeof(new_sa2));
	new_sa2.sa_handler = &signal_handler3;
	sigaction(SIGINT, &new_sa2, &old_sa2);

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

void signal_handler3(int signo){
	msgq = msgget(key, IPC_CREAT | 0666);
	memset(&msg,0,sizeof(msg),0,NULL);
	ret = msgrcv(msgq, &msg, sizeof(msg),0,NULL);

	struct p_PCB* r_PCB;
	r_PCB = pop_queue(wait_q);
	for(int i=0; i<msg.io_time;i++){
		printf("I/O work, remaining time:  (%d) \n",msg.io_timei-i);}
	printf("(%d) process's I/O burst finish",msg.pid);

	add_queue(&run_q,r_PCB->pid,r_PCB->burst_time);
	
	if (count == total_CPU_burst_time && IsEmpty(&wait_q)){
 		printf("CPU burst and I/O busrt are all finish, so scheduleing is end\n");
		exit(0);
	}

}

void signal_handler2(int signo)
{
	printf("(%d) SIGALRM signaled!\n", getpid());
	count++;
	if(count%t_quantum==0){
		msgq = msgget(key,IPC_CREAT | 0666);
		memset(&msg, 0, sizeof(msg));
		msg.mtype =0;
		msg.pid=getpid();
		msg.io_time = rand()%10+5;
		ret = msgsnd(msgq, &msg, sizeof(msg),NULL);


		kill(parent_pid, SIGINT);
	}

	if(count ==total_exec_time){
		printf("(%d)  execution completed@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n",getpid());
		}
}

void signal_handler(int signo)
{
	struct p_PCB* r_PCB = malloc(sizeof(p_PCB));
	r_PCB = run_q.front;
	int target_pid = r_PCB->pid;
	printf("(%d)->(%d) signal! count: %d \n", getpid(), target_pid, count);
	// send child a signal SIGUSR1
	kill(target_pid, SIGALRM);
	r_PCB->burst_time -= 1;
	count++;
	if(r_PCB->burst_time == 0){
		r_PCB = pop_queue(&run_q);
		free(r_PCB);
	}
	else if (r_PCB->time_quantum == 0) {
		r_PCB = pop_queue(&run_q);
		add_queue(&wait_q,r_PCB->pid,r_PCB->burst_time);
	}
	else{
		r_PCB->time_quantum -= 1;
	}

	if (count == total_CPU_burst_time){
		printf("Total CPU work finished!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");	
		}
}

void add_queue(struct Run_q* run_q,int pid, int burst) {

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode->pid = pid;
	newNode->burst_time = burst;
	newNode->state= 0;
	newNode->remaining_wait= 0;
	newNode->next = NULL;
	newNode->time_quantum = 2;
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
