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
int total_exec_time = 0; // used by child proc.

int pids[5];
int time_quantum[5];

struct proc_t {
    int pid;
    int state;
    int remaining_tq;
    int remaining_wait;
};

struct procq_t {
    struct proc_t* p;
    struct procq_t* next;
};

struct procq_t* runq = NULL;
//struct procq_t* waitq = NULL;

void add_queue(struct procq_t* target, struct proc_t* p) {
    struct procq_t* newNode = malloc(sizeof(struct procq_t));
    //처음에는  runq노드에 헤더가 없으므로, 첫 번째 노드가 헤더의 역할을 한>다.
    if (target == NULL) {
        target = newNode;
        target->next = NULL;
        target->p = p;
    }
    // 두 번째부터
    else {
        newNode->next = target->next;
        newNode->p = p;
        target->next = newNode;
    }

}
int main()
{
    for (int i = 0; i < 5; i++) {
        pids[i] = 0;
        time_quantum[i] = 0;
    }
    // child fork
    for (int i = 0; i < 5; i++) {
        int ret = fork();
        if (ret < 0) {
            // fork fail
            perror("fork_failed");
        }
        else if (ret == 0) {
            // child

            // signal handler setup
            struct sigaction old_sa;
            struct sigaction new_sa;
            memset(&new_sa, 0, sizeof(new_sa));
            new_sa.sa_handler = &signal_handler2;
            sigaction(SIGALRM, &new_sa, &old_sa);

            // execution time
            total_exec_time = 2;
            while (1);
            exit(0);
            // never reach here
        }
        else if (ret > 0) {
            // parent
            pids[i] = ret;
            printf("child %d created\n", pids[i]);

            // alloc new pcb
            struct proc_t* p = malloc(sizeof(struct proc_t));
            p->pid = ret;
            // put it in the runq
            add_queue(runq, p);
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
    count++;
    if (count == total_exec_time) {
        printf("(%d) execution completed!\n", getpid());
        exit(0);
    }
}


void signal_handler(int signo)
{

    int target_pid = pids[count % 5];
    printf("(%d)->(%d) signal! count: %d \n", getpid(), target_pid, count);
    // send child a signal SIGUSR1
    kill(target_pid, SIGALRM);
    count++;

    //      if (count == total_exec_time){
    if (count == 10) {
        struct procq_t* target = runq;
        // clen up resources & children
        // for all children
        printf("\n");
        for (target = runq; target != NULL; target = runq->next) {
            kill(target->p->pid, SIGKILL);
            free(target);
        }
        free(runq);
        printf("\nFINISHED!\n");
        exit(0);
    }
}

