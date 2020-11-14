/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "msg.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

typedef struct p_PCB {
	int pid;
	int burst_time;
	int remaining_wait;
	int time_quantum;
	int state;
	int io_wh;
	struct p_PCB* next;
}p_PCB;

typedef struct Run_q {
	p_PCB* front;
	p_PCB* rear;
	int count;
}Run_q;

void signal_handler(int signo);
void signal_handler2(int signo);
void add_queue(struct Run_q* run_q, int pid, int burst, int wait, int when, int state);
void InitQueue(struct Run_q* run_q);
int IsEmpty(struct Run_q* run_q);
struct p_PCB* pop_queue(struct Run_q* run_q);
int count = 0;
int wait = 0;
int total_exec_time = 0;//used by child
int exec_time = 0;
int pids[10];
int time_quantum[10];
int io_timing[10];
int total_CPU_burst_time = 0;
int IO_when = 0;
int io_wh = 0;

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
	for (int i = 0; i < 10; i++) {
		pids[i] = 0;
		time_quantum[i] = (rand() % 11) + 10; //10부터 20까지
		//io_timing[i] = (rand() % 6) + 4; //4부터 9까지
	}
	// child fork
	for (int i = 0; i < 10; i++) {
		pipe(fd);
		int burst = 0;
		int ret = fork();
		if (ret < 0) {
			// fork fail
			perror("fork_failed");
		}
		else if (ret == 0) {
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
			exec_time = burst;
			//io_wh = io_timing[i];

			write(fd[1], &burst, sizeof(burst));
			while (1);
			exit(0);
			// never reach here
		}
		else if (ret > 0) {
			// parent
			close(fd[1]);
			read(fd[0], &burst, sizeof(burst));
			total_CPU_burst_time = total_CPU_burst_time + burst;
			pids[i] = ret;
			//InitQueue(run_q);
			add_queue(&run_q, pids[i], burst, 0, 0, 0);
			printf("child %d created, %d\n", pids[i], burst);
		}
	}


	printf("total cpu burst time is %d\n", total_CPU_burst_time);

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
	srand(time(NULL));
	printf("---handler2 start---\n");
	printf("  H2: (%d) SIGALRM signaled!\n", getpid());
	if (count == 0) {
		
						//실행 시간으로 나눈 나머지를 io작업 시작 시점으로 하자.
						//1부터 exectime-1까지.
						io_wh = (rand()%6)+4;

						int msgq;
						int ret;
						int key = 0x12345;
						msgq = msgget( key, IPC_CREAT | 0666);
						printf("SEND: msgq id: %d\n", msgq);

						struct msgbuf msg;
						memset(&msg, 0, sizeof(msg));
						msg.mtype = 0;
						msg.pid = getpid();
						msg.io_when = io_wh;
				ret = msgsnd(msgq, &msg, sizeof(msg), 0);
				printf("msgsnd ret: %d\n", ret);

	}
	//waitq 상태에서
	if (wait != 0) {
		wait -= 1;
		printf("  H2: WAITQUE_%d, REMAINING_TIME_%d\n", getpid(), wait);
	}
	//runq 상태에서
	else {
		count++;
		//만약 count가 IO-when과 같다면 메세지 작업 시작
		if (count < io_wh) {
			printf("  H2: first_exec\n");
		}
		else if (count == io_wh) {
			//wait queue 진입
			printf("  H2: wait queue\n");
			wait = rand() % 10 + 1;

			int msgq;
			int ret;
			int key = 0x12345;
			msgq = msgget(key, IPC_CREAT | 0666);
			printf("SEND: msgq id: %d\n", msgq);

			struct msgbuf msg;
			memset(&msg, 0, sizeof(msg));
			msg.mtype = 0;
			msg.pid = getpid();
			msg.io_time = wait;
			ret = msgsnd(msgq, &msg, sizeof(msg), 0);
			printf("msgsnd ret: %d\n", ret);
		}
		else if (count < exec_time) {
			printf("  H2: second_exec\n");
		}
		else if (count == exec_time) {
			printf("  H2: (%d) execution completed@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n", getpid());
			printf("---handler2 end---\n");
			exit(0);
		}
	}
	printf("---handler2 end---\n");
}

void signal_handler(int signo)
{
	int msgq;
	printf("---handler1 start---\n");
	if (!IsEmpty(&wait_q)) {
		struct p_PCB* w_PCB = malloc(sizeof(p_PCB));
		w_PCB = wait_q.front;
		int target_pid2 = w_PCB->pid;
		printf("   H1: WAIT(%d): %d\n", target_pid2, w_PCB->remaining_wait);
		kill(target_pid2, SIGALRM);
		w_PCB->remaining_wait -= 1;
		if (w_PCB->remaining_wait == 0) {
			w_PCB = pop_queue(&wait_q);
			add_queue(&run_q, w_PCB->pid, w_PCB->burst_time, w_PCB->remaining_wait, w_PCB->io_wh, 2);
		}

	}
	if (!IsEmpty(&run_q)) {
		struct p_PCB* r_PCB = malloc(sizeof(p_PCB));
		r_PCB = run_q.front;
		int target_pid = r_PCB->pid;
		printf("   H1: (%d)->(%d) signal! count: %d \n", getpid(), target_pid, count);

                // send child a signal SIGUSR1
                kill(target_pid, SIGALRM);

		//처음 실행하는거면 (state==0), 언제 io를 실행하는지에 대한 메세지를 받자.
		//불완전하지만 부모가 아닌 자녀가 io 시간을 정하는 것을 구현하려 하였다.
		if (r_PCB->state== 0) {
			
									int ret;
									int key = 0x12345;
									msgq = msgget( key, IPC_CREAT | 0666);
									printf("msgq id: %d\n", msgq);

									struct msgbuf msg;
									memset(&msg, 0, sizeof(msg));
									ret = msgrcv(msgq, &msg, sizeof(msg), 0, 0);
									printf("msgsnd ret: %d\n", ret);
									printf("msg.mtype: %d\n", msg.mtype);
									printf("msg.pid: %d\n", msg.pid);
									printf("msg.io_when: %d\n", msg.io_when);
									//                        printf("msg.next_exec_time: %d\n", msg.next_exec_time);
			
			r_PCB->io_wh = msg.io_when;
			r_PCB->state = 1;
		}
		count++;
		r_PCB->burst_time -= 1;
		r_PCB->io_wh -= 1;

		//끝
		if ((r_PCB->burst_time == 0) & (r_PCB->state == 2)) {
			r_PCB = pop_queue(&run_q);
			free(r_PCB);
		}

		//waitq진입
		else if ((r_PCB->io_wh == 0) & (r_PCB->state == 1)) {
			//wait queue 진입

			printf("TQ is zero: PID: %d\n", getpid());
			//                      int msgq;
			int ret;
			int key = 0x12345;
			msgq = msgget(key, IPC_CREAT | 0666);
			printf("msgq id: %d\n", msgq);

			struct msgbuf msg;
			memset(&msg, 0, sizeof(msg));
			ret = msgrcv(msgq, &msg, sizeof(msg), 0, 0);
			printf("msgsnd ret: %d\n", ret);
			printf("msg.mtype: %d\n", msg.mtype);
			printf("msg.pid: %d\n", msg.pid);
			printf("msg.io_time: %d\n", msg.io_time);
			//printf("msg.next_exec_time: %d\n", msg.next_exec_time);
			r_PCB = pop_queue(&run_q);
			add_queue(&wait_q, r_PCB->pid, r_PCB->burst_time, msg.io_time, r_PCB->io_wh, r_PCB->state);
			//                      r_PCB->remaining_wait = 5;
			//                      r_PCB->remaining_wait = msg.io_time;
		}

		//tq끝남
		else if (r_PCB->time_quantum == 0) {
			r_PCB = pop_queue(&run_q);
			add_queue(&run_q, r_PCB->pid, r_PCB->burst_time, 0, r_PCB->io_wh, r_PCB->state);
		}
		//그게 아니면
		else {
			r_PCB->time_quantum -= 1;
		}
	}
	//      if (count == total_CPU_burst_time){
	//              printf("   H1: finished!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	//      }
	if ((count == total_CPU_burst_time) & (IsEmpty(&run_q)) & (IsEmpty(&wait_q))) {
		printf("******************************REALLY BYE~~\n");
		msgctl(msgq, IPC_RMID, 0);
		exit(0);
	}
	printf("---handler1 end---\n");
}
void add_queue(struct Run_q* run_q, int pid, int burst, int wait, int when, int state) {

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode->pid = pid;
	newNode->burst_time = burst;
	newNode->remaining_wait = wait;
	newNode->next = NULL;
	newNode->time_quantum = 2;
	newNode->state = state;
	newNode->io_wh = when;
	if (IsEmpty(run_q)) {
		run_q->front = run_q->rear = newNode;
	}
	else {
		run_q->rear->next = newNode;
		run_q->rear = newNode;
	}

	run_q->count++;
}

struct p_PCB* pop_queue(struct Run_q* run_q) {

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode = run_q->front;
	run_q->front = newNode->next;
	run_q->count--;
	return newNode;
}
void InitQueue(struct Run_q* run_q) {

	run_q->front = run_q->rear = NULL;
	run_q->count = 0;
}
int IsEmpty(struct Run_q* run_q) {

	if (run_q->count == 0)
		return 1;
	else
		return 0;
}

