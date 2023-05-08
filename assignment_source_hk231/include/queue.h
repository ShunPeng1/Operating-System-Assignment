
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 10

struct queue_t {
	struct pcb_t * proc[MAX_QUEUE_SIZE];
	int size;
#ifdef MLQ_SCHED
	int slot;
#endif
};

#ifdef MLQ_SCHED
struct mlq_t {
	struct queue_t queues[MAX_PRIO];
	int proc_count;
	int slot_count;
};
#endif // MLQ_SCHED

void enqueue(struct queue_t *q, struct pcb_t *proc, struct mlq_t *mlq);

struct pcb_t * dequeue(struct queue_t * q);

int empty(struct queue_t * q);

#endif

