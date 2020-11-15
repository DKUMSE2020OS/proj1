import os 
import time
import threading
import random
import signal
import copy 
import multiprocessing

total_time = multiprocessing.Value('d',0)
end = multiprocessing.Value('d',0)

shared_cpu_burst = multiprocessing.Value('d',0)
shared_io_burst = multiprocessing.Value('d',0)
shared_c_turn = multiprocessing.Value('d',0)
shared_i_turn = multiprocessing.Value('d',0)

class PCB:
    c_turn =0
    i_turn =0

    pid =0

    cpu_burst =0
    io_burst =0

def play():
    if total_time.value == 100:
        exit(0)
    signal.signal(signal.SIGUSR1,child)
    while True:
        time.sleep(1)

def child(a,b):
    if shared_c_turn.value == 1:
        shared_cpu_burst.value-=1
        os.kill(os.getppid(),signal.SIGUSR2)
        
    elif shared_i_turn.value ==1:
        shared_io_burst.value-=1
        os.kill(os.getppid(),signal.SIGUSR2)

def arrange(a,b):
    if shared_c_turn.value == 1:
        ready_q[0].cpu_burst = shared_cpu_burst.value
        shared_c_turn.value = 0
        ready_q[0].c_turn = shared_c_turn.value
        
    elif shared_i_turn.value == 1:
        wait_q[0].io_burst = shared_io_burst.value
        shared_i_turn.value = 0
        wait_q[0].i_turn = shared_i_turn.value
    
    if ready_q[0].cpu_burst == 0:
        tmp = ready_q.pop(0)
        wait_q.append(tmp)
            

    if wait_q!=[]:
        if wait_q[0].io_burst ==0 :
            tmp = wait_q.pop(0)
            tmp.cpu_burst = random.randint(8,15)
            tmp.io_burst = random.randint(3,15)
            ready_q.append(tmp)


    end.value = 1


def parent():
    signal.signal(signal.SIGUSR2,arrange)
    display()
    tr =0
    if end.value == 0 and ready_q[0].c_turn == 0:
        ready_q[0].c_turn = 1
        shared_cpu_burst.value = ready_q[0].cpu_burst
        shared_c_turn.value = ready_q[0].c_turn
        os.kill(ready_q[0].pid,signal.SIGUSR1)
        while end.value == 0:
            tr =0
        end.value =0
    
    if wait_q != [] and wait_q[0].i_turn ==0:
        wait_q[0].i_turn =1
        shared_io_burst.value = wait_q[0].io_burst
        shared_i_turn.value = wait_q[0].i_turn
        os.kill(wait_q[0].pid,signal.SIGUSR1)
        while end.value == 0:
            tr =0
        end.value =0
    
    total_time.value+=1


def ready_init(pid):
    new_pcb = PCB()
    new_pcb.pid = pid
    new_pcb.cpu_burst = random.randint(4,8)
    new_pcb.io_burst = random.randint(4,8)
    ready_q.append(new_pcb)

def display():
    print('******************ready_q****************************')
    for x in range(len(ready_q)):
        print('id : ',ready_q[x].pid,'cpu :',ready_q[x].cpu_burst ,'io : ',ready_q[x].io_burst)
    print('******************wait_q****************************')
    if wait_q != []:
        for x in range(len(wait_q)):
            print('id : ',wait_q[x].pid,'cpu :',wait_q[x].cpu_burst,'io :',wait_q[x].io_burst)
    print('\n\n')



if __name__=='__main__':
    ready_q = []
    wait_q = []

    for x in range(10):
        pid = os.fork()
        if pid == 0:
            play()
        else:
            ready_init(pid)
    
    while total_time.value != 500:
        parent()
        time.sleep(0.3)
        

    exit(0)