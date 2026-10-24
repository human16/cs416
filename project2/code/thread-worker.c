// File:	thread-worker.c

// List all group member's name: Ben Yavin, May Takahashi
// username of iLab: by240
// iLab Server: cs.ilab.rutgers.edu

#include "thread-worker.h"
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define TERMINATED 3
#define QUANTUM 100
#define MAX_THREADS 1024
#define BOOST_INTERVAL_MS (10 * QUANTUM)

long tot_cntx_switches = 0;
double avg_turn_time = 0;
double avg_resp_time = 0;

static double total_turn_time;
static double total_resp_time;
static int completed_threads;
#ifndef MLFQ
static node *runqueue_head;
#else
static node *mlfq_queues[MLFQ_LEVELS];
#endif
static worker_t worker_id_counter = 1;
static ucontext_t *scheduler_context;
static tcb *curr_tcb;
static tcb main_tcb;
static tcb *all_threads[MAX_THREADS];
static int scheduler_reason;
static struct timespec last_boost_cpu;
static int scheduler_initialized;

enum {
	SCHED_NONE,
	SCHED_PREEMPT,
	SCHED_YIELD,
	SCHED_BLOCK,
	SCHED_EXIT
};

static void schedule(void);
static void worker_trampoline(uintptr_t worker_address);

static double elapsed_ms(const struct timespec *start,
                         const struct timespec *end)
{
	return (end->tv_sec - start->tv_sec) * 1000.0 +
	       (end->tv_nsec - start->tv_nsec) / 1000000.0;
}

#ifdef MLFQ
static void cpu_now(struct timespec *now)
{
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, now);
}
#endif

static void wall_now(struct timespec *now)
{
	clock_gettime(CLOCK_MONOTONIC, now);
}

static void stop_timer(void)
{
	struct itimerval timer = {0};
	setitimer(ITIMER_PROF, &timer, NULL);
}

static void arm_timer(int milliseconds)
{
	struct itimerval timer = {0};
	timer.it_value.tv_sec = milliseconds / 1000;
	timer.it_value.tv_usec = (milliseconds % 1000) * 1000;
	if (timer.it_value.tv_sec == 0 && timer.it_value.tv_usec == 0)
		timer.it_value.tv_usec = 1;
	setitimer(ITIMER_PROF, &timer, NULL);
}

#ifndef MLFQ
static void psjf_enqueue(node **head, worker_t thread_id, tcb *thread_tcb)
{
	node *new_node = calloc(1, sizeof(*new_node));
	if (new_node == NULL)
		abort();
	new_node->thread_id = thread_id;
	new_node->thread_tcb = thread_tcb;
	if (*head == NULL ||
	    (*head)->thread_tcb->counter > thread_tcb->counter) {
		new_node->next = *head;
		*head = new_node;
		return;
	}
	node *current = *head;
	while (current->next != NULL &&
	       current->next->thread_tcb->counter <= thread_tcb->counter)
		current = current->next;
	new_node->next = current->next;
	current->next = new_node;
}
#endif

static void mlfq_enqueue(node **head, worker_t thread_id, tcb *thread_tcb)
{
	node *new_node = calloc(1, sizeof(*new_node));
	if (new_node == NULL)
		abort();
	new_node->thread_id = thread_id;
	new_node->thread_tcb = thread_tcb;
	if (*head == NULL) {
		*head = new_node;
		return;
	}
	node *current = *head;
	while (current->next != NULL)
		current = current->next;
	current->next = new_node;
}

static node *dequeue(node **head)
{
	node *result = *head;
	if (result != NULL) {
		*head = result->next;
		result->next = NULL;
	}
	return result;
}

#ifdef MLFQ
static int mlfq_slice_ms(int priority)
{
	return QUANTUM * (1 << priority);
}
#endif

static void enqueue_ready(tcb *thread)
{
	if (thread == NULL || thread->status == TERMINATED)
		return;
	thread->status = READY;
#ifdef MLFQ
	if (thread->priority < 0)
		thread->priority = 0;
	if (thread->priority >= MLFQ_LEVELS)
		thread->priority = MLFQ_LEVELS - 1;
	mlfq_enqueue(&mlfq_queues[thread->priority], thread->thread_id, thread);
#else
	psjf_enqueue(&runqueue_head, thread->thread_id, thread);
#endif
}

static void sigprof_handler(int signal_number, siginfo_t *info, void *context)
{
	(void)signal_number;
	(void)info;
	(void)context;
	if (!curr_tcb || curr_tcb == &main_tcb ||
	    curr_tcb->status != RUNNING || scheduler_context == NULL)
		return;
	stop_timer();
	scheduler_reason = SCHED_PREEMPT;
	swapcontext(curr_tcb->thread_context, scheduler_context);
}

static void initialize_scheduler(void)
{
	struct sigaction action = {0};
	action.sa_sigaction = sigprof_handler;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	if (sigaction(SIGPROF, &action, NULL) != 0)
		abort();

	memset(&main_tcb, 0, sizeof(main_tcb));
	main_tcb.thread_id = 0;
	main_tcb.status = RUNNING;
	if (getcontext(&main_tcb.context_storage) == -1)
		abort();
	main_tcb.thread_context = &main_tcb.context_storage;
	curr_tcb = &main_tcb;

	ucontext_t *context = calloc(1, sizeof(*context));
	void *stack = malloc(SIGSTKSZ);
	if (context == NULL || stack == NULL)
		abort();
	if (getcontext(context) == -1)
		abort();
	context->uc_link = NULL;
	context->uc_stack.ss_sp = stack;
	context->uc_stack.ss_size = SIGSTKSZ;
	makecontext(context, schedule, 0);
	scheduler_context = context;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &last_boost_cpu);
	scheduler_initialized = 1;
}

static void account_current(void)
{
	struct timespec now;
	if (curr_tcb == NULL || curr_tcb == &main_tcb)
		return;
	wall_now(&now);
#ifdef MLFQ
	cpu_now(&now);
	curr_tcb->level_cpu_ms += elapsed_ms(&curr_tcb->dispatch_cpu_start, &now);
	if (curr_tcb->level_cpu_ms >= mlfq_slice_ms(curr_tcb->priority)) {
		if (curr_tcb->priority < MLFQ_LEVELS - 1)
			curr_tcb->priority++;
		curr_tcb->level_cpu_ms = 0;
	}
#endif
}

#ifdef MLFQ
static void maybe_boost(void)
{
	struct timespec now;
	cpu_now(&now);
	if (elapsed_ms(&last_boost_cpu, &now) < BOOST_INTERVAL_MS)
		return;
	last_boost_cpu = now;
	for (int i = 1; i < MLFQ_LEVELS; ++i) {
		node *current;
		while ((current = dequeue(&mlfq_queues[i])) != NULL) {
			current->thread_tcb->priority = 0;
			current->thread_tcb->level_cpu_ms = 0;
			mlfq_enqueue(&mlfq_queues[0], current->thread_id,
			             current->thread_tcb);
			free(current);
		}
	}
	for (int i = 1; i < MAX_THREADS; ++i) {
		if (all_threads[i] != NULL &&
		    all_threads[i]->status == BLOCKED) {
			all_threads[i]->priority = 0;
			all_threads[i]->level_cpu_ms = 0;
		}
	}
}
#endif

#ifndef MLFQ
static tcb *next_psjf(void)
{
	node *entry = dequeue(&runqueue_head);
	if (entry == NULL)
		return NULL;
	tcb *result = entry->thread_tcb;
	free(entry);
	return result;
}
#endif

#ifdef MLFQ
static tcb *next_mlfq(void)
{
	for (int i = 0; i < MLFQ_LEVELS; ++i) {
		node *entry = dequeue(&mlfq_queues[i]);
		if (entry != NULL) {
			tcb *result = entry->thread_tcb;
			free(entry);
			return result;
		}
	}
	return NULL;
}
#endif

static void dispatch_thread(tcb *next)
{
	struct timespec now;
	curr_tcb = next;
	next->status = RUNNING;
	wall_now(&now);
	if (next != &main_tcb && !next->responded) {
		total_resp_time += elapsed_ms(&next->start_time, &now);
		next->responded = 1;
		if (completed_threads > 0)
			avg_resp_time = total_resp_time / completed_threads;
	}
	tot_cntx_switches++;
	scheduler_reason = SCHED_NONE;
#ifdef MLFQ
	cpu_now(&next->dispatch_cpu_start);
	arm_timer(mlfq_slice_ms(next->priority) -
	          (int)next->level_cpu_ms);
#else
	arm_timer(QUANTUM);
#endif
	swapcontext(scheduler_context, next->thread_context);
}

static void schedule(void)
{
	for (;;) {
		stop_timer();
		account_current();
#ifdef MLFQ
		maybe_boost();
#endif
		if (scheduler_reason == SCHED_PREEMPT ||
		    scheduler_reason == SCHED_YIELD)
			enqueue_ready(curr_tcb);

		tcb *next;
#ifdef MLFQ
		next = next_mlfq();
#else
		next = next_psjf();
#endif
		if (next == NULL) {
			if (main_tcb.status == READY || main_tcb.status == RUNNING) {
				dispatch_thread(&main_tcb);
				return;
			}
			continue;
		}
		dispatch_thread(next);
	}
}

static void worker_trampoline(uintptr_t worker_address)
{
	tcb *worker = (tcb *)worker_address;
	curr_tcb = worker;
	worker_exit(worker->thread_function(worker->thread_arg));
}

int worker_create(worker_t *thread, pthread_attr_t *attr,
                  void *(*function)(void *), void *arg)
{
	(void)attr;
	if (!scheduler_initialized)
		initialize_scheduler();
	if (worker_id_counter >= MAX_THREADS || function == NULL)
		return -1;

	tcb *worker = calloc(1, sizeof(*worker));
	ucontext_t *context = calloc(1, sizeof(*context));
	void *stack = malloc(SIGSTKSZ);
	if (worker == NULL || context == NULL || stack == NULL)
		abort();
	worker->thread_id = worker_id_counter++;
	worker->status = READY;
	worker->thread_function = function;
	worker->thread_arg = arg;
	worker->thread_context = context;
	worker->start_time.tv_sec = 0;
	wall_now(&worker->start_time);
	worker->priority = 0;
	if (getcontext(context) == -1)
		abort();
	context->uc_link = NULL;
	context->uc_stack.ss_sp = stack;
	context->uc_stack.ss_size = SIGSTKSZ;
	makecontext(context, (void (*)(void))worker_trampoline, 1,
	            (uintptr_t)worker);
	all_threads[worker->thread_id] = worker;
	enqueue_ready(worker);
	*thread = worker->thread_id;
	return 0;
}

int worker_yield(void)
{
	stop_timer();
	scheduler_reason = SCHED_YIELD;
	swapcontext(curr_tcb->thread_context, scheduler_context);
	return 0;
}

void worker_exit(void *value_ptr)
{
	if (curr_tcb == &main_tcb)
		return;
	stop_timer();
	curr_tcb->return_value = value_ptr;
	curr_tcb->status = TERMINATED;
	struct timespec now;
	wall_now(&now);
	total_turn_time += elapsed_ms(&curr_tcb->start_time, &now);
	completed_threads++;
	avg_resp_time = total_resp_time / completed_threads;
	avg_turn_time = total_turn_time / completed_threads;
	if (curr_tcb->joining_thread != NULL) {
		tcb *joining = curr_tcb->joining_thread;
		curr_tcb->joining_thread = NULL;
		enqueue_ready(joining);
	}
	scheduler_reason = SCHED_EXIT;
	setcontext(scheduler_context);
	abort();
}

int worker_join(worker_t thread, void **value_ptr)
{
	if (thread >= MAX_THREADS || all_threads[thread] == NULL ||
	    curr_tcb == all_threads[thread])
		return -1;
	tcb *target = all_threads[thread];
	if (target->status != TERMINATED) {
		if (target->joining_thread != NULL)
			return -1;
		curr_tcb->status = BLOCKED;
		target->joining_thread = curr_tcb;
		stop_timer();
		scheduler_reason = SCHED_BLOCK;
		swapcontext(curr_tcb->thread_context, scheduler_context);
	}
	if (value_ptr != NULL)
		*value_ptr = target->return_value;
	all_threads[thread] = NULL;
	free(target->thread_context->uc_stack.ss_sp);
	free(target->thread_context);
	free(target);
	return 0;
}

int worker_mutex_init(worker_mutex_t *mutex,
                      const pthread_mutexattr_t *mutexattr)
{
	(void)mutexattr;
	mutex->lock = 0;
	mutex->blocked_threads = NULL;
	return 0;
}

int worker_mutex_lock(worker_mutex_t *mutex)
{
	while (__sync_lock_test_and_set(&mutex->lock, 1)) {
		stop_timer();
		curr_tcb->status = BLOCKED;
		mlfq_enqueue(&mutex->blocked_threads, curr_tcb->thread_id, curr_tcb);
		scheduler_reason = SCHED_BLOCK;
		swapcontext(curr_tcb->thread_context, scheduler_context);
	}
	return 0;
}

int worker_mutex_unlock(worker_mutex_t *mutex)
{
	__sync_lock_release(&mutex->lock);
	node *entry;
	while ((entry = dequeue(&mutex->blocked_threads)) != NULL) {
		enqueue_ready(entry->thread_tcb);
		free(entry);
	}
	return 0;
}

int worker_mutex_destroy(worker_mutex_t *mutex)
{
	if (mutex->blocked_threads != NULL)
		return -1;
	if (mutex->lock != 0)
		return -1;
	return 0;
}

/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void)
{
	fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
	fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
	fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}
