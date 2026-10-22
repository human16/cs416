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
#include <sys/time.h>

#define READY 0
#define RUNNING 1
#define BLOCKED 2

#define QUANTUM 100 

#define MAX_THREADS 100

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;

double total_turn_time=0;
double total_resp_time=0;
double num_threads=0;


// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
node *runqueue_head = NULL;

worker_t worker_id_counter = 0; // Counter for assigning unique thread IDs

// Where the scheduler is at
ucontext_t *scheduler_context = NULL;

// My current worker tcb
tcb *curr_tcb = NULL;

tcb *all_threads[MAX_THREADS]; 


// queue functions

//enqueue function for PSJF, sorted by counter least to greatest
void psjf_enqueue(node **head, worker_t thread_id, tcb *thread_tcb) {
	node *new_node = (node *)malloc(sizeof(node));
	new_node->thread_id = thread_id;
	new_node->thread_tcb = thread_tcb;
	new_node->next = NULL;

	if (*head == NULL) {
		*head = new_node;
	} else {
		node *temp = *head;
		while (temp->next != NULL) {
			if (temp->thread_tcb->counter <= thread_tcb->counter) {  // I think I implemented it correctly, idk though
				temp=temp->next; 
			} else {
				break;
			}
		}
		new_node->next = temp->next;
		temp->next = new_node;
	}
}

//
void mlfq_enqueue(node **head, worker_t thread_id, tcb *thread_tcb) {
	node *new_node = (node *)malloc(sizeof(node));
	new_node->thread_id = thread_id;
	new_node->thread_tcb = thread_tcb;
	new_node->next = NULL;

	if (*head == NULL) {
		*head = new_node;
	} else {
		node *temp = *head;
		while (temp->next != NULL) {
			temp=temp->next; 
		}
		temp->next = new_node;
	}
}

node *dequeue(node **head) {
	if (*head == NULL) {
		return NULL;
	}

	node *temp = *head;
	*head = (*head)->next;
	temp->next = NULL;
	return temp;
}

int is_empty(node *head) {
	return head == NULL;
}

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
		// |||||||||||||||||||||||||||||||
        //        scheduler setup
	    // |||||||||||||||||||||||||||||||


		// Bunch of setup for scheduler tcb
		tcb *scheduler_tcb = (tcb *)malloc(sizeof(tcb));
		scheduler_tcb->thread_id = worker_id_counter;
		worker_id_counter++;
		scheduler_tcb->status = RUNNING; // Scheduler is always running
		scheduler_tcb->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
		scheduler_tcb->thread_function = NULL;
		scheduler_tcb->thread_arg = NULL;
		getcontext(scheduler_tcb->thread_context);
		
		//after getting the context, setting up
		scheduler_context = scheduler_tcb->thread_context;
		scheduler_context->uc_link = NULL;
		void *stack = malloc(SIGSTKSZ);
		scheduler_context->uc_stack.ss_sp = stack;
		scheduler_context->uc_stack.ss_size = SIGSTKSZ;
		scheduler_context->uc_stack.ss_flags = 0;
		makecontext(scheduler_context, (void (*)(void))schedule, 0);

		//sets the first context to the scheduler
		curr_tcb = scheduler_tcb;

		// |||||||||||||||||||||||||||||||
        //           timer setup
	    // |||||||||||||||||||||||||||||||

		struct sigaction sa;
		memset (&sa, 0, sizeof (sa));
		// sa.sa_handler = &ring;
		sigaction (SIGPROF, &sa, NULL);

		// Create timer struct
		struct itimerval timer;

		// Set up what the timer should reset to after the timer goes off
		timer.it_interval.tv_usec = 0; 
		timer.it_interval.tv_sec = 0;

		// Set up the current timer to go off in 1 second
		// Note: if both of the following values are zero
		//       the timer will not be active, and the timer
		//       will never go off even if you set the interval value
		timer.it_value.tv_usec = QUANTUM*1000;;
		timer.it_value.tv_sec = 1;

		// Set the timer up (start the timer)
		setitimer(ITIMER_PROF, &timer, NULL);
	}

	num_threads++; 

	// setting up worker tcb
	tcb *worker_tcb = (tcb *)malloc(sizeof(tcb));
	worker_tcb->thread_id = worker_id_counter;
	*thread = worker_id_counter;
	worker_id_counter++;
	worker_tcb->status = READY; 
	worker_tcb->counter = 0; 
	worker_tcb->start_time = time(NULL);
	worker_tcb->thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
	
	getcontext(worker_tcb->thread_context);
	
	// setting up stack for worker tcb context
	void *stack=malloc(SIGSTKSZ);
	worker_tcb->thread_context->uc_link=NULL;
	worker_tcb->thread_context->uc_stack.ss_sp=stack;
	worker_tcb->thread_context->uc_stack.ss_size=SIGSTKSZ;
	worker_tcb->thread_context->uc_stack.ss_flags=0;

	worker_tcb->thread_function = function;
	worker_tcb->thread_arg = arg;
	makecontext(worker_tcb->thread_context, (void (*)(void))function, 1, arg);

	// Add to runqueue
	psjf_enqueue(&runqueue_head, worker_tcb->thread_id, worker_tcb);
    return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	// tot_cntx_switches++; move to scheduler.

	// just like, give up control bro
	curr_tcb->status = READY; 
	psjf_enqueue(&runqueue_head, curr_tcb->thread_id, curr_tcb); 
	swapcontext(curr_tcb->thread_context, scheduler_context);

	// need to reenter queue (runqueue_head)
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE
	if (value_ptr != NULL) {
		curr_tcb->return_value = value_ptr; // Store return value
	}
	curr_tcb->status = BLOCKED;
	total_turn_time += difftime(time(NULL), curr_tcb->start_time);
	avg_turn_time = total_turn_time / num_threads;


	if (curr_tcb->joining_thread != NULL) {
		// If there is a joining thread, unblock it
		curr_tcb->joining_thread->status = READY;
		psjf_enqueue(&runqueue_head, curr_tcb->joining_thread->thread_id, curr_tcb->joining_thread);
	}

	// free(curr_tcb->thread_context->uc_stack.ss_sp); 
	// curr_tcb->thread_context->uc_stack.ss_sp = NULL; // Free stack memory
	// free(curr_tcb->thread_context);
	// curr_tcb->thread_context = NULL; // Free context memory
	// free(curr_tcb);
	// curr_tcb = NULL;

	setcontext(scheduler_context); 
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	tcb *joining_tcb = NULL;
	// YOUR CODE HERE
	for (int i = 0; i < MAX_THREADS; i++) {
		if (all_threads[i] != NULL && all_threads[i]->thread_id == thread) {
			joining_tcb = all_threads[i];
			break;
		}
	}
	if (joining_tcb == NULL) {
		return -1;
	}

	if (joining_tcb->status == RUNNING || joining_tcb->status == READY) {
		curr_tcb->status = BLOCKED;
		joining_tcb->joining_thread = curr_tcb;
		swapcontext(curr_tcb->thread_context, scheduler_context);
	}

	if (value_ptr != NULL) {
		*value_ptr = joining_tcb->return_value;
	}

	all_threads[joining_tcb->thread_id] = NULL;
	if (joining_tcb->thread_context != NULL) {
		free(joining_tcb->thread_context->uc_stack.ss_sp);
		free(joining_tcb->thread_context);
	}
	free(joining_tcb);

	thread_exit(NULL);

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
		curr_tcb->status = BLOCKED;
		mlfq_enqueue(&mutex->blocked_threads, curr_tcb->thread_id, curr_tcb);
		swapcontext(curr_tcb->thread_context, scheduler_context);
	}
	// mutex->lock = 1;
	return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	while(mutex->blocked_threads != NULL) {
		node *unblocked_node = dequeue(&mutex->blocked_threads);
		unblocked_node->thread_tcb->status = READY;
		psjf_enqueue(&runqueue_head, unblocked_node->thread_id, unblocked_node->thread_tcb);
		free(unblocked_node);
	}

	__sync_lock_release(&mutex->lock); 

	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init
	if (mutex->lock == 1 || worker(mutex->blocked_threads) != NULL) {
		worker_mutex_unlock(mutex);
	}

	mutex->lock = 0;
	mutex->blocked_threads = NULL;
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

