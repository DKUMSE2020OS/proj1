MLFQ_sched : MLFQ_sched.o
	gcc -o MLFQ_sched MLFQ_sched.o

MLFQ_sched.o : MLFQ_sched.c
	gcc -c -o MLFQ_sched.o MLFQ_sched.c

clean :
	rm *.o MLFQ_sched

