#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

/* Thuan
Put the proc into the queue, the queue is an array so we add at size-1, then size++
*/
void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        q->proc[q->size] = proc;
        q->size++;
}

/* Thuan
Get the proc at index 0, then shift the rest to the left;
*/
struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if(empty(q)) return NULL;

        struct pcb_t *result = q->proc[0];
        for(int i = 1; i<q->size; i++){
                q->proc[i-1] = q->proc[i]; // shift to the left
        }

        q->size--;
        q->proc[q->size] = NULL;

        return result;
}

