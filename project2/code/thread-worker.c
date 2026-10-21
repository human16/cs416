// File:	thread-worker.c

// List all group member's name: Ben Yavin, May Takahashi
// username of iLab: by240
// iLab Server: 

#include "thread-worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <ucontext.h>


//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;


// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
node *runqueue_head = NULL;

worker_t worker_id_counter = 0; // Counter for assigning unique thread IDs

// Where the scheduler is at
ucontext_t *scheduler_context = NULL;

// My current worker tcb
tcb *curr_tcb = NULL;

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

       // - create Thread Control Block (TCB)
       // - create and initialize the context of this worker thread
       // - allocate space of stack for this thread to run
       // - after everything is set, push this thread into run queue and 
       // - make it ready for the execution.

       // YOUR CODE HERE
	if (scheduler_context == NULL) 
	{
		scheduler_context = (ucontext_t *)malloc(sizeof(ucontext_t));
		getcontext(scheduler_context);
		
		scheduler_context->uc_link = NULL;
		void *stack = malloc(SIGSTKSZ);
		scheduler_context->uc_stack.ss_sp = stack;
		scheduler_context->uc_stack.ss_size = SIGSTKSZ;
		scheduler_context->uc_stack.ss_flags = 0;
		makecontext(scheduler_context, (void (*)(void))schedule, 0);
	}


	tcb *worker_tcb = (tcb *)malloc(sizeof(tcb));
	worker_tcb->thread_id = worker_id_counter;
	*thread = worker_id_counter;
	worker_id_counter++;
	worker_tcb->status = 0; 
	worker_tcb->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
	
	getcontext(worker_tcb->thread_context);
	
	void *stack=malloc(SIGSTKSZ);
	worker_tcb->thread_context->uc_link=NULL;
	worker_tcb->thread_context->uc_stack.ss_sp=stack;
	worker_tcb->thread_context->uc_stack.ss_size=SIGSTKSZ;
	worker_tcb->thread_context->uc_stack.ss_flags=0;

	worker_tcb->thread_function = function;
	worker_tcb->thread_arg = arg;
	makecontext(worker_tcb->thread_context, (void (*)(void))function, 1, arg);

	// Add to runqueue

    return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	// tot_cntx_switches++; move to scheduler.
	curr_tcb->status = 0; // Change status to Ready
	swapcontext(curr_tcb->thread_context, scheduler_context);

	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE


};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	

	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	mutex->lock = 0;
	mutex->blocked_threads = NULL;
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
		while (__sync_lock_test_and_set(&mutex->lock, 1)){
			worker_yield();
		}
		mutex->lock = 1;
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	mutex->lock = 0;
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	getcontext(scheduler_context);

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// - schedule policy
#ifndef MLFQ
	// Choose PSJF
#else 
	// Choose MLFQ
#endif

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

