/*
    OS Project #1 SimpleScheduling
    Group
    32161570 Park Kitae
    32162066 Byun Sangun

    CPU -> RunQ, Round Robin with Time Quantum 20
    I/O -> WaitQ, FCFS

    Compile : gcc proj1.c -o proj1
    View Result : Cat result.txt
*/
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define buffer_size 0x300   //Buffer Size
#define Time_Q 20           //Time_Quantum

#include "msg.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char buffer[buffer_size];

struct Proc_t {
        pid_t pid;
        int burst;
        int time_quantum;
        struct Proc_t *next;    // Next Node
};

struct Queue_t{   //Run_Queue
        struct Proc_t* front;
        struct Proc_t* rear;
        int count;
};

struct Queue_t runq_t;           // Tick: 1sec
struct Queue_t waitq_t;          // Running = 0, Ready = 1, Wait = 2

int fd;
int count = 0;
int state = 0;

int random_burst_time = 0;      // Burst time per Process
int random_io_time = 0;         // I/O Time per Process

int total_burst_time = 0;       // Total Burst Time
int total_io_time = 0;          // Total I/O Time

pid_t pids[10];                 // PID per Process
int process_time[10];           // Burst Time per Process
int io_time[10];                // I/O Time per Process

void init_queue(struct Queue_t q);
void add_queue(struct Queue_t *rq, int pid, int burst, int tq);
struct Proc_t* pop_queue(struct Queue_t* rq);
void signal_handler(int signo);
void signal_handler2(int signo);


void init_queue(struct Queue_t q){   //Initialize Queue
        q.front = NULL;
        q.rear = NULL;
        q.count = 0;
}

void add_queue(struct Queue_t *rq, int pid, int burst, int tq){

        struct Proc_t *temp = malloc(sizeof(struct Proc_t));

        temp->pid = pid;
        temp->burst = burst;
        temp->time_quantum = tq;
        temp->next = NULL;

        if(rq->count == 0){     //Queue Empty or not?
                rq->front = temp;
                rq->rear = temp;
        } else {
                rq->rear->next = temp;
                rq->rear = temp;
        }

        rq->count++;
}

struct Proc_t* pop_queue(struct Queue_t* rq){   //Ruturn Process 

        struct Proc_t* temp = malloc(sizeof(struct Proc_t));

        temp = rq->front;
        rq->front = temp->next;
        rq->count--;

        return temp;
}



int main()
{
        printf("-------------PROJECT #1 SIMPLE SCHEDULING-------------\n");
        printf("-------------32161570 Kitae Park----------------------\n");
        printf("-------------32162066 Sangun Byun---------------------\n\n");
        printf("------------------------------------------------------\n\n");

        //Open file 'result.txt'
        fd = open("result.txt", O_CREAT | O_WRONLY | O_TRUNC, 0600);

        if(fd == -1){
            perror("Failed to Create txt file.\n");
            exit(0);
        }

        srand(time(NULL));
        //Init Queue
        init_queue(runq_t);
        init_queue(waitq_t);

        for (int i = 0 ; i < 10; i++) {
                pids[i] = 0;    // Initialize PID
                process_time[i] = rand() % 40 + 60;  // Set Burst TIme
        }

        // Child Fork
        for (int i = 0 ; i < 10; i++) {

                //int burst = 0;
                int ret = fork();
                
                if (ret < 0) {  // Fork failed
                        perror("fork_failed");

                } else if (ret == 0) {  // Child

                        // Signal handler setup
                        struct sigaction old_sa;
                        struct sigaction new_sa;
                        memset(&new_sa, 0, sizeof(new_sa));
                        new_sa.sa_handler = &signal_handler2;                        
                        sigaction(SIGALRM, &new_sa, &old_sa);
                        // Execution Time
                        random_burst_time = process_time[i]; 

                        //Repeating
                        while (1);
                        exit(0); // Never reach here

                } else if (ret > 0) {   // Parent

                        total_burst_time += process_time[i];
                        pids[i] = ret;
                        printf("Child %d created, BT: %dsec\n", pids[i], process_time[i]);

                        // Put it in the RunQ
                        add_queue(&runq_t, pids[i], process_time[i], Time_Q-1);
                }
        }

        printf("-------------------Now Scheduling---------------------\n\n");
        printf("-------------------Please Wait---------------------\n\n");

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
        close(fd);
        return 0;
}

void signal_handler2(int signo){    //Signal Child(User)->Parent(OS)

        count++;

        if(count == random_burst_time){
            //printf("-------------Child (%d) CPU Execution Completed!-------------\n\n", getpid());

            srand(time(NULL));

            random_io_time = rand() % 40 + 80;    //Set I/O time 80 ~ 120

            int msgq;
            int ret;
            int key = 0x12345;
            msgq = msgget( key, IPC_CREAT | 0666);      // Create MSG

            struct msgbuf msg;
            memset(&msg, 0, sizeof(msg));
            msg.mtype = 0;
            msg.pid = getpid();
            msg.io_time = random_io_time;
            ret = msgsnd(msgq, &msg, sizeof(msg), 0);   // Send MSG
        }

        else if(count == random_burst_time + random_io_time){           
            //printf("---------------Child (%d) I/O Execution Completed!------------\n\n", getpid());
            exit(0);
        }
}


void signal_handler(int signo)  //Signal Parent(OS)->Child(User)
{
        count++;
        // printf("\n———————————Scheduler——————————\n");
        // printf("Count : %d\n", count);
        // printf("total_burst_time : %d \n", total_burst_time);        
        // printf("total_io_time : %d \n\n", total_io_time);

        sprintf(buffer, "\n———————————Scheduler——————————\n");
        sprintf(buffer, "%sCount : %d\n", buffer, count);
        sprintf(buffer, "%stotal_burst_time : %d \n", buffer, total_burst_time);
        sprintf(buffer, "%stotal_io_time : %d \n\n", buffer, total_io_time);
        

        int burst = 0;  //Remaining Burst Time
        int wait = 0;   //Remaining I/O Time

        // Run queue
        if(runq_t.count != 0){

            struct Proc_t* p = malloc(sizeof(struct Proc_t));
            p = runq_t.front;
            int target_pid = p->pid;

            // printf(" CPU Burst : Parent (%d) -> Child (%d)\n", getpid(), target_pid);
            sprintf(buffer, "%s CPU Burst : Parent (%d) -> Child (%d)\n", buffer, getpid(), target_pid);


            // Send child a signal SIGUSR1
            kill(target_pid, SIGALRM);

            p->burst -= 1;

            // In RunQ, consume all burst time
            if(p->burst == 0){  
                
                p = pop_queue(&runq_t);


                int msgq;
                int ret;
                int key = 0x12345;
                msgq = msgget( key, IPC_CREAT | 0666);
            
                struct msgbuf msg;
                memset(&msg, 0, sizeof(msg));
                ret = msgrcv(msgq, &msg, sizeof(msg) ,0, 0);

                total_io_time += msg.io_time;
                add_queue(&waitq_t, msg.pid, msg.io_time, 0); 

                // printf("\t->Remain_burst_time : %d\n", burst);
                sprintf(buffer, "%s\t->Remain_burst_time : %d\n", buffer, burst);

                // printf("\t->Move child(%d) to Wait_Queue\n", target_pid);
                sprintf(buffer, "%s\t->Move child(%d) to Wait_Queue\n", buffer, target_pid);

                free(p);
            }
            // In RunQ, consume all time quantum
            else if (p->time_quantum == 0) {
                p = pop_queue(&runq_t);
                add_queue(&runq_t, p->pid, p->burst, Time_Q-1);
                burst = p->burst;

                // printf("\t->Remain_burst_time : %d\n", burst);
                sprintf(buffer, "%s\t->Remain_burst_time : %d\n", buffer, burst);
            }
       
            // In RunQ, remain both time quantum and burst time
            else {
                p->time_quantum -= 1;
                burst = p->burst;
                
                // printf("\t->Remain_burst_time : %d\n", burst);
                sprintf(buffer, "%s\t->Remain_burst_time : %d\n", buffer, burst);
            }

            //printf("\t->Remain_burst_time : %d\n", burst);
            //sprintf(buffer, "%s\t->Remain_burst_time : %d\n", buffer, burst);

        } else {    // NO CPU burst
            // printf(" NO CPU Burst Action!\n");
            sprintf(buffer, "%s NO CPU Burst Action!\n", buffer);
        }

    
        // Wait queue
        if(waitq_t.count != 0) {
            struct Proc_t* io = malloc(sizeof(struct Proc_t));
            io = waitq_t.front;
            int target_pid_io = io->pid;

            // printf(" I/O burst : Parent (%d) -> Child (%d)\n", getpid(), target_pid_io);
            sprintf(buffer, "%s I/O burst : Parent (%d) -> Child (%d)\n", buffer, getpid(), target_pid_io);

            // send child a signal SIGUSR1
            kill(target_pid_io, SIGALRM);
            io->burst -= 1;
            wait = io->burst;

            // printf("\t->Remain_I/O_time : %d\n", wait);
            sprintf(buffer, "%s\t->Remain_I/O_time : %d\n", buffer, wait);

            if(io->burst == 0) {    //IO time 끝났을 떄
                pop_queue(&waitq_t);
                // printf("\t->Child(%d) I/O Finished\n", target_pid_io);
                sprintf(buffer, "%s\t->Child(%d) I/O Finished\n", buffer, target_pid_io);
            } 

        } else {
            // printf(" NO I/O Execution\n");
            sprintf(buffer, "%s NO I/O Execution\n", buffer);
        }


    struct Proc_t* temp = malloc(sizeof(struct Proc_t));
    temp = runq_t.front;


    //Print Current Status of Run Queue
    // printf("\nRUN  QUEUE -> ");
    sprintf(buffer, "%s\nRUN  QUEUE -> ", buffer);

    for(int i = 0; i < runq_t.count; i++) {
            // printf(" [%d] ", temp->pid);
            sprintf(buffer, "%s [%d] ", buffer, temp->pid);
            temp = temp->next;
    }
        
    // printf("\n\n");
    sprintf(buffer, "%s\n\n", buffer);


    //Print current Status of Wait Queue
    // printf("\nWAIT QUEUE -> ");
    sprintf(buffer, "%s\nWAIT  QUEUE -> ", buffer);

    struct Proc_t* temp2 = malloc(sizeof(struct Proc_t));
    temp2 = waitq_t.front;

    for(int i = 0; i < waitq_t.count; i++) {
        // printf(" [%d] ", temp2->pid);
        sprintf(buffer, "%s [%d] ", buffer, temp2->pid);
        temp2 = temp2->next;
    }
    // printf("\n\n");
    sprintf(buffer, "%s\n\n", buffer);

    // printf("\nTime Quantum : %d\n", Time_Q);
    // printf("Wait_Queue count: %d \n", waitq_t.count);
    // printf("Run_Queue count: %d \n", runq_t.count);
    // printf("——————————————————————————————\n");

    sprintf(buffer, "%s\nTime Quantum : %d\n", buffer, Time_Q);
    sprintf(buffer, "%sWait_Queue count: %d \n", buffer, waitq_t.count);
    sprintf(buffer, "%sRun_Queue count: %d \n", buffer, runq_t.count);
    sprintf(buffer, "%s——————————————————————————————\n", buffer);

    // Finish Scheduling
    if (count > total_burst_time && waitq_t.count == 0 && runq_t.count == 0 ) {
        printf("—————————————————————————————————————————————————————\n");
        printf("————————————Result are saved as result.txt———————————\n");
        printf("—————————————————————————————————————————————————————\n");
        sprintf(buffer, "%s—————————————————————Programme END———————————————————\n", buffer);
        sprintf(buffer, "%s—————————————————————————————————————————————————————\n", buffer);
        write(fd, buffer, sizeof(buffer)-1);
        memset(buffer, 0 , sizeof(buffer));
        exit(0);
    }

    write(fd, buffer, sizeof(buffer));
    memset(buffer, 0, sizeof(buffer));
}
