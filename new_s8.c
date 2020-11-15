/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#define t_quantum 2
struct msgbuf {

	// pid will sleep for io_time
	int pid;
	int io_time;
	int next_exec_time;
	int io_when;
};

typedef struct p_PCB {
	int pid; //프로세스의 ID
	int burst_time; //프로세스가 각각 실행되는 총 시간.
	int remaining_wait; //waitq에서 탈출(?)하기 위해 카운트되는 변수. 한 번에 1씩 줄어들며 0이 되면 waitq에서 빠져나와 runq로 들어간다.
	int time_quantum; //runq의 round robbin 구현을 위해 사용되는 tq. 이 값이 addq에서 2로 설정됨에 따라 한 프로세스별로 최대 3번의 기회가 주어진다.
	int state; //state가 0이면 io처리가  되지 않은 상태, 1은  io가  처리가 된 상태. (코드상 io처리는 1번만 진행된다)
	int io_timer; //프로세스가 io작업을 시작하기까지 필요한 시간. 이 값이 0이 된다면 프로세스는 runq에서 빠져나와 waitq로 넘어간다.
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
void add_queue_first(struct Run_q* run_q, int pid, int burst, int wait, int when, int state);
void InitQueue(struct Run_q* run_q);
int IsEmpty(struct Run_q* run_q);
struct p_PCB* pop_queue(struct Run_q* run_q);

int pids[10]; //used in main
int time_quantum[10]; //used in main
int count = 0; //both used
int exec_time = 0; //used by child
int total_exec_time = 0;//used by child
int io_when[10]; //used by child
int total_CPU_burst_time = 0; //used by parent
int wait = 0;
int child_timing = 0;

struct Run_q run_q; //CPU
struct Run_q wait_q; //IO

FILE* fp=NULL;

int main()
{

	fp = fopen("schedule_dump.txt", "w");
	if(fp==NULL){
		perror("can't make dump text");}


	int fd[2];
	int fd1[2];
	srand(time(NULL));
	InitQueue(&run_q);
	InitQueue(&wait_q);
	for (int i = 0; i < 10; i++) {
		pids[i] = 0;
		time_quantum[i] = (rand() % 1000) + 500; //4부터 12까지. 4부터 시작하는 이유는 tq가 3이며, msg가 혼잡해지는 경우를 방지하기 위함. (나도 잘 모르겠.. 4밑으로 하면 프로세스가 안죽더라구)
		io_when[i] = (rand()%(time_quantum[i]-1))+1;
	}
	// child fork
	for (int i = 0; i < 10; i++) {
		pipe(fd);
		pipe(fd1);
		int burst = 0;
		int timing = 0;
		int ret = fork();
		if (ret < 0) {
			// fork fail
			perror("fork_failed");
		}
		else if (ret == 0) {
			// child

			close(fd[0]);
			close(fd1[0]);
			// signal handler setup
			struct sigaction old_sa;
			struct sigaction new_sa;
			memset(&new_sa, 0, sizeof(new_sa));
			new_sa.sa_handler = &signal_handler2;
			sigaction(SIGALRM, &new_sa, &old_sa);

			//excution time
			burst = time_quantum[i];
			exec_time = burst;
			write(fd[1], &burst, sizeof(burst));

			timing = io_when[i];
			child_timing = timing;
			write(fd1[1], &timing, sizeof(timing));

			while (1);
			exit(0);
			// never reach here
		}
		else if (ret > 0) {
			// parent

			close(fd[1]);
			close(fd1[1]);
			read(fd[0], &burst, sizeof(burst));
			read(fd1[0], &timing, sizeof(timing));

			total_CPU_burst_time = total_CPU_burst_time + burst;
			pids[i] = ret;
			add_queue(&run_q, pids[i], burst, 0, timing, 0);
			fprintf(fp,"child %d created, exec %d, timing %d\n", pids[i], burst, timing);
		}
	}

	fprintf(fp,"total cpu burst time is %d\n", total_CPU_burst_time);

	// signal handler setup
	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	// fire the alrm timer
	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = 50000;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	while (1);
	return 0;
}


void signal_handler2(int signo)
{
	srand(time(NULL));
	count++;
	//io작업 전 일반적인 상황
	if (count == child_timing) {
		//wait queue 진입
		wait = rand() % 100 + 1;
		int msgq;
		int ret;
		int key = 0x23456;
		msgq = msgget(key, IPC_CREAT | 0666);
		struct msgbuf msg;
		memset(&msg, 0, sizeof(msg));
		msg.pid = getpid();
		msg.io_time = wait;
		ret = msgsnd(msgq, &msg, sizeof(msg), 0);
	}
	else if (count == (exec_time+wait)) {
//		printf("SIGNAL2(%d) END\n", getpid());
		exit(0);
	}
}

void signal_handler(int signo)
{
	int msgq;
	int msgq2;

	//waitq가 비어있지 않을 때
	if (!IsEmpty(&wait_q)) {
		struct p_PCB* w_PCB = malloc(sizeof(p_PCB));
		w_PCB = wait_q.front;
		int target_pid2 = w_PCB->pid;
		fprintf(fp,"WAIT_Q(%d) remaining_io_time: %d\n", target_pid2, w_PCB->remaining_wait);
		kill(target_pid2, SIGALRM);
		if (w_PCB->remaining_wait == 0) {
			w_PCB = pop_queue(&wait_q);
			add_queue_first(&run_q, w_PCB->pid, w_PCB->burst_time, w_PCB->remaining_wait, w_PCB->io_timer, 1);
		}
		w_PCB->remaining_wait -= 1;
	}
	//runq가 비어있지 않을 때
	if (!IsEmpty(&run_q)) {
		struct p_PCB* r_PCB = malloc(sizeof(p_PCB));
		r_PCB = run_q.front;
		int target_pid = r_PCB->pid;
		// send child a signal SIGUSR1
		kill(target_pid, SIGALRM);
		count++;
		r_PCB->burst_time -= 1;
		r_PCB->io_timer -= 1;
		//프로세스 종료
		if ((r_PCB->burst_time == 0) & (r_PCB->state == 1)) {
			fprintf(fp,"RUN _Q(%d) count: %d, process end\n", target_pid, count);
			r_PCB = pop_queue(&run_q);
			free(r_PCB);
		}
		//io_timer가 0이 된 순간. child로부터 io처리되는데 걸리는 시간에 관련된 메세지를 받고, waitq로 진입한다.
		else if ((r_PCB->io_timer == 0) & (r_PCB->state == 0)) {
			fprintf(fp,"RUN _Q(%d) count: %d, go to i.o\n", target_pid, count);
			int ret;
			int key = 0x23456;
			msgq = msgget(key, IPC_CREAT | 0666);

			struct msgbuf msg;
			memset(&msg, 0, sizeof(msg));
			ret = msgrcv(msgq, &msg, sizeof(msg), 0, 0);
			r_PCB = pop_queue(&run_q);
			add_queue(&wait_q, r_PCB->pid, r_PCB->burst_time, msg.io_time, r_PCB->io_timer, r_PCB->state);
		}
		//tq끝나서 맨 뒤로 돌아감
		else if (r_PCB->time_quantum == 0) {
			fprintf(fp,"RUN _Q(%d) count: %d, tq: 0. go to end\n", target_pid, count);
			r_PCB = pop_queue(&run_q);
			add_queue(&run_q, r_PCB->pid, r_PCB->burst_time, 0, r_PCB->io_timer, r_PCB->state);
		}
		//아무 상황도 아닌 평범한 상황
		else {
			fprintf(fp,"RUN _Q(%d) count: %d, tq: %d.\n", target_pid, count, r_PCB->time_quantum);
			r_PCB->time_quantum -= 1;
		}
	}
	if ((count == total_CPU_burst_time) && (IsEmpty(&run_q)) && (IsEmpty(&wait_q))) {
	
		fprintf(fp,"FINISHED\n");
		msgctl(msgq, IPC_RMID, 0);
		msgctl(msgq2, IPC_RMID, 0);
		fclose(fp);
		exit(0);
	}
}

void add_queue(struct Run_q* run_q, int pid, int burst, int wait, int when, int state) {

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode->pid = pid;
	newNode->burst_time = burst;
	newNode->remaining_wait = wait;
	newNode->next = NULL;
	newNode->time_quantum = t_quantum;
	newNode->state = state;
	newNode->io_timer = when;
	if (IsEmpty(run_q)) {
		run_q->front = run_q->rear = newNode;
	}
	else {
		run_q->rear->next = newNode;
		run_q->rear = newNode;
	}

	run_q->count++;
}

void add_queue_first(struct Run_q* run_q, int pid, int burst, int wait, int when, int state) {

	struct p_PCB* newNode = malloc(sizeof(p_PCB));

	newNode->pid = pid;
	newNode->burst_time = burst;
	newNode->remaining_wait = wait;
	newNode->next = NULL;
	newNode->time_quantum = 2;
	newNode->state = state;
	newNode->io_timer = when;
	if (IsEmpty(run_q)) {
		run_q->front = run_q->rear = newNode;
	}
	else {
		newNode->next = run_q->front;
		run_q->front = newNode;
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
