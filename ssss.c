/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

typedef struct p_PCB{
    int pid;
    int state;
    int burst_time;
    int remaining_wait;
    struct p_PCB *next;
}p_PCB;

typedef struct Run_q{
     p_PCB* front;
     p_PCB* rear;
     int count;
}Run_q;

void signal_handler(int signo);
void signal_handler2(int signo);
void add_queue(struct Run_q* run_q,int pid,int burst);
void InitQueue(struct Run_q *run_q);
int IsEmpty(struct Run_q* run_q);
struct p_PCB* pop_queue(struct Run_q* run_q);
int count = 0;
int total_exec_time =0;//used by child
int pids[10];
int time_quantum[5];
int total_CPU_burst_time=0;

struct Run_q run_q;
//struct p_PCB* PCB[10];
//struct p_PCB r_PCB;

int main()
{
	int fd[2];
	srand(time(NULL));
	InitQueue(&run_q);

	for (int i = 0 ; i < 10; i++) {
		pids[i] = 0;
		time_quantum[i] = rand()%10+1;
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
			
			//InitQueue(run_q);
			add_queue(&run_q,pids[i],burst);
                        printf("child %d created, %d\n", pids[i],burst);
		}
	}


	/*for(int i=0; i<10; i++){
		PCB[i]= pop_queue(&run_q);
		printf("%dst | id:(%d) | burst+time:(%d)\n",i,PCB[i]->pid,PCB[i]->burst_time);
	}*/

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
		printf("(%d) execution completed@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n",getpid());
		exit(0);}
}

void signal_handler(int signo)
{
	
	struct p_PCB* r_PCB = pop_queue(&run_q);
	int target_pid = r_PCB->pid;
	printf("(%d)->(%d) signal! count: %d \n", getpid(), target_pid, count);
	// send child a signal SIGUSR1
	kill(target_pid, SIGALRM);
	count++;
	r_PCB->burst_time -= 1;
	if(r_PCB->burst_time ==0)
		free(r_PCB);
	else
		add_queue(&run_q,r_PCB->pid,r_PCB->burst_time);

	if (count == total_CPU_burst_time) exit(0);
}

void add_queue(struct Run_q* run_q,int pid, int burst) {
    
    struct p_PCB* newNode = malloc(sizeof(p_PCB));
   
    newNode->pid = pid;
    newNode->burst_time = burst;
    newNode->state= 0;
    newNode->remaining_wait= 0;
    newNode->next = NULL;

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
