
#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

#ifdef MLQ_SCHED
static struct mlq_t mlq_ready_queue;
#endif

// return 0 as long as there is 1 pcb, return 1 when it is empty
int queue_empty(void) {
#ifdef MLQ_SCHED
	return (mlq_ready_queue.proc_count == 0);
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++)
	{
		mlq_ready_queue.queues[i].size = 0;
		mlq_ready_queue.queues[i].slot = MAX_PRIO - i;
	}

	mlq_ready_queue.slot_count = (MAX_PRIO * (MAX_PRIO + 1)) / 2;
	mlq_ready_queue.proc_count = 0;
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
// Quang added this
void refill_slots_of_mlq(void)
{
	for (int i = 0; i < MAX_PRIO; i++)
	{
		mlq_ready_queue.queues[i].slot = MAX_PRIO - i;
	}

	mlq_ready_queue.slot_count = (MAX_PRIO * (MAX_PRIO + 1)) / 2;

#if SCHED_DBG
	printf("DEBUG: REFILL SLOT\n");
#endif
}

struct pcb_t *get_proc_by_slot(){
	if (mlq_ready_queue.proc_count == 0)
	{
		return NULL;
	}
	for(int i = 0 ; i < MAX_PRIO; i++){
		if( !empty(&mlq_ready_queue.queues[i]) && (mlq_ready_queue.queues[i].slot > 0) ) {
			mlq_ready_queue.queues[i].slot--;
			return dequeue(&mlq_ready_queue.queues[i]);
		}
	}
	return NULL;
}

/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	
	/* Thuan
	return NULL when queue is empty, find the highest priority non-empty queue and dequeue
	*/
	
	pthread_mutex_lock(&queue_lock);

	if(!queue_empty()) {
		for(int i = 0 ; i < MAX_PRIO; i++){
			if( !empty(&mlq_ready_queue.queues[i]) && (mlq_ready_queue.queues[i].slot > 0) ) {
				proc = dequeue(&mlq_ready_queue.queues[i]);
				
				mlq_ready_queue.queues[i].slot--;
				mlq_ready_queue.slot_count--;
				mlq_ready_queue.proc_count--;
				
				if (mlq_ready_queue.proc_count == 0 || mlq_ready_queue.slot_count == 0)
				{
					refill_slots_of_mlq();
				}
				break;
			}
		}
	}
	pthread_mutex_unlock(&queue_lock);
	return proc;	
}

// enqueue a proc to mlq_queue base on it priority
void put_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue.queues[proc->prio], proc, &mlq_ready_queue);
	pthread_mutex_unlock(&queue_lock);
}

// enqueue a proc to mlq_queue base on it priority
void add_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue.queues[proc->prio], proc, &mlq_ready_queue);
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	return proc;
}

void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif


