#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#define key 0x12345
#define R_TIME 3
#define MAX_CHILD 10  
#define MAX_DURATION_TIME 500

int msgq;
int ret;
struct msgbuf msg;
int cpu_burst;
int io_burst;
int total_counter=0;
int ready_count=0;
int wait_count=0;
int turn=0;
int time_q;

struct msgbuf {
	int mtype;
	int pid;
	int io_time;
	int message;
};

typedef struct pcb{
    struct pcb* next;
    int pid;
	int io_burst;
	int cpu_burst;
	int cnt_wait;
	int cnt_cpu;
	int cnt_io;

}pcb;

typedef struct Q{
    pcb *front;
    pcb *rear;
}make_q;
make_q* ready_q;
make_q* wait_q;

make_q* init_q();
void set_message(int input);
void get_message();
void ready_q_push(pcb* n_pcb);
void ready_q_pop();
void wait_q_push(pcb* n_pcb);
void wait_q_pop();
void io_countdown();
void signal_f();
void ready_init(int pid);
void show_monitor();
void set_rand_time_init(int child_num);
void per_time_duration(int signo);
void change_q_call(int signo);
void ch_do_cpu();
void get_cputime(int signo);
void refresh_node(int signo);
void count_wait_q();
void small();
void middle();
void large();
void finish();
void dup_process();

///////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *arg[])
{
	msgq = msgget(key, IPC_CREAT | 0666);
	memset(&msg, 0, sizeof(msg));

    ready_q = init_q();
	wait_q = init_q();
        
	signal_f();
	dup_process();
    while(1){}

    return 0;

}
////////////////////////////////////////////////////////////////////////////////////////////////////////
void dup_process(){
        for(int i = 0; i < MAX_CHILD; i++){
            int pid = fork();
            if(pid == 0){
				set_rand_time_init(i);
                ch_do_cpu();
                exit(0);
                }
            else{
				time_q=R_TIME;
                ready_init(pid);
				ready_count++;
            }
        }
}
void per_time_duration(int signo)
{	total_counter++;
	show_monitor();
	finish();
	if(wait_count!=0){io_countdown();}
	count_wait_q();
	if(turn==0){
		set_message(time_q);
		turn=1;
	}
	ready_q->front->cnt_cpu++;
    kill(ready_q->front->pid, SIGUSR1);
    printf("Duration time %d :parent send message to (%d)\n", total_counter, ready_q->front->pid);
	
}

void signal_f(){

		struct sigaction old_sa, new_sa;
        memset(&new_sa, 0, sizeof(new_sa));
        new_sa.sa_handler = per_time_duration;
        sigaction(SIGALRM, &new_sa, &old_sa);

        struct sigaction old_sa_siguser2, new_sa_siguser2;
        memset(&new_sa_siguser2, 0, sizeof(new_sa_siguser2));
        new_sa_siguser2.sa_handler = refresh_node;
        sigaction(SIGUSR2, &new_sa_siguser2, &old_sa_siguser2);

        struct sigaction old_sa_siguser3, new_sa_siguser3;
        memset(&new_sa_siguser3, 0, sizeof(new_sa_siguser3));
        new_sa_siguser3.sa_handler = change_q_call;
        sigaction(SIGUSR1, &new_sa_siguser3, &old_sa_siguser3);

 		struct itimerval new_timer, old_timer;
        memset(&new_timer, 0, sizeof(new_timer));
		new_timer.it_interval.tv_sec = 0;
   		new_timer.it_interval.tv_usec = 200*1000;
   		new_timer.it_value.tv_sec = 0;
   		new_timer.it_value.tv_usec = 2000*100;
        setitimer(ITIMER_REAL, &new_timer, &old_timer);


}
void ch_do_cpu()
{
        struct sigaction old_sa_siguser, new_sa_siguser;
        memset(&new_sa_siguser, 0, sizeof(new_sa_siguser));
        new_sa_siguser.sa_handler = get_cputime;
        sigaction(SIGUSR1, &new_sa_siguser, &old_sa_siguser);

        while(1){}
}

void get_cputime(int signo)
{	
	if(turn==0){
	get_message();
	time_q=msg.message;
	turn=1;
	}
    cpu_burst--;
    time_q--;
    printf("ME (%d) using CPU remain time : %d\n", getpid(), cpu_burst);
    if(cpu_burst == 0){
		set_rand_time_init((rand()%10)+1);
		turn=0;
		set_message(io_burst);
        kill(getppid(), SIGUSR1);
    }
    else if(time_q == 0){
        turn=0;
        kill(getppid(), SIGUSR2);
    }
}

make_q* init_q(){
        make_q *Q;
        Q = (make_q*)malloc(sizeof(make_q));
        Q->front = NULL;
        Q->rear = NULL;
        return Q;
}

void change_q_call(int signo)
{		
    ready_q_pop();
	turn=0;
}

void refresh_node(int signo)
{
    ready_q->front = ready_q->front->next;
	ready_q->rear = ready_q->rear->next;
	turn=0;
}

void ready_q_pop()
{

    pcb* temp;
	temp= ready_q->front;

	if(ready_q->front==ready_q->rear){
		ready_count--;
		get_message();
		temp->io_burst=msg.message;
		ready_q->front=NULL;
		ready_q->rear=NULL;
		wait_q_push(temp);
		return;
	}
	else{
		ready_q->front = ready_q->front->next;
		ready_q->rear->next = ready_q->front;
		ready_count--;
		get_message();
		temp->io_burst=msg.message;
		wait_q_push(temp);
        return;
	}
}
void wait_q_pop(){
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        temp = wait_q->front;
	
    wait_q->front = wait_q->front->next;
	ready_q_push(temp);
	wait_count--;
}


void ready_q_push(pcb* n_pcb){
	if(ready_q->front==NULL){
		ready_q->front=n_pcb;
	}
	else{
		ready_q->rear->next=n_pcb;
	}

	ready_q->rear=n_pcb;
	ready_q->rear->next=ready_q->front;	

	ready_count++;
}

void wait_q_push(pcb* n_pcb){
	int newbi;
	int oldbi;
    pcb* temp;
    temp = (pcb*)malloc(sizeof(pcb));
    temp = wait_q->front;    
	if(wait_q->front==NULL){
		wait_q->front=n_pcb;
		n_pcb->next=NULL;
		wait_count++;
		return;
	}
	 if(n_pcb->io_burst<=temp->io_burst)
	 {
	   wait_q->front=n_pcb;
	   n_pcb->next=temp;
       wait_count++;
	   return;
	 }	
	while(temp->next!=NULL){
		newbi=n_pcb->io_burst;
		oldbi=temp->next->io_burst;
		if(newbi<=oldbi){break;}
        temp = temp->next;
        }
    n_pcb->next = temp->next;
    temp->next = n_pcb;
	wait_count++;
}

void ready_init(int pid)
{
        pcb *n_pcb;
        n_pcb = (pcb*)malloc(sizeof(pcb));
        n_pcb->pid = pid;

        n_pcb->next = NULL;

        if(ready_q->front == NULL){
                ready_q->front = n_pcb;
                n_pcb->next = n_pcb;
                ready_q->rear = n_pcb;
        }
        else{
        	ready_q->rear->next = n_pcb;
        	ready_q->rear = n_pcb;
        	n_pcb->next = ready_q->front;
        }

}
void io_countdown()
{
    pcb* temp;
    temp= wait_q->front;
    for(int i=0;i<wait_count;i++){
		temp->io_burst--;
		temp->cnt_io++;
		if(temp->io_burst==0){wait_q_pop();}
		temp=temp->next;
    }
}

void count_wait_q(){
 	pcb* temp;
 	if(ready_count<2){return;}
 	else{
 		temp=ready_q->front->next;
  		while(temp!=ready_q->front){
   			temp->cnt_wait++;
   			temp=temp->next;
  		}
	}
}

void show_monitor()
{
	pcb* temp;
	temp=ready_q->front;
	printf("\n\n##################################################################\n\n");
	printf("Ready_q list [ ");
	for(int k=0;k<ready_count;k++){
		printf("ID:%d[%d %c] ",temp->pid,100*(temp->cnt_cpu)/total_counter,37);
		temp=temp->next;
	}
	printf("]\n\n");
	temp=wait_q->front;
	printf("wait_q list [ ");
	for(int k=0;k<wait_count;k++){
		printf("ID:%d[%d %c] ",temp->pid,100*(temp->cnt_cpu)/total_counter,37);
		temp=temp->next;
	}
	printf("]\n\n");
}

void set_rand_time_init(int child_num)
{
 if (child_num<=3){
	 small();
 }
 else if(child_num<=6){
	 middle();
 }
 else{large();}
}

void small(){
	cpu_burst = (rand()%10)+(rand()%3);
	io_burst = (rand()%10)+(rand()%9);
}
void middle(){
	cpu_burst = (rand()%10)+(rand()%5);
	io_burst = (rand()%10)+(rand()%5);
}
void large(){
	cpu_burst = (rand()%10)+(rand()%9);
	io_burst = (rand()%10)+(rand()%3);
}
void set_message(int input)
{
	msg.mtype=0;
	msg.pid=getpid();
	msg.message=input;
	ret=msgsnd(msgq,&msg,sizeof(msg),0);
}
void get_message(){ret = msgrcv(msgq,&msg,sizeof(msg),0,0);}
void finish(){if(total_counter>MAX_DURATION_TIME){exit(0);}}