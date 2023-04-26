#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

/* Thuan
Put the proc into the queue, the queue is an array so we add at size-1, then size++
*/

void swap(struct pcb_t *a, struct pcb_t *b)
{
        struct pcb_t *temp = a;
        a = b;
        b = temp;
}
void sortQueue(struct queue_t *q)
{
        int swapped;
        if (q == NULL)
        {
                return;
        }
        for (int i = 0; i < q->size - 1; i++)
        {
                struct pcb_t *maxItem = q->proc[i];
                for (int j = i; j < q->size; j++)
                {
                        if (q->proc[j]->priority > maxItem->priority)
                        {
                                maxItem = q->proc[j];
                        }
                }
        }
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        if (q->size < MAX_QUEUE_SIZE)
        {
                q->proc[q->size] = proc;
                q->size++;
        }
        sortQueue(q);
        /* TODO: put a new process to queue [q] */
}

/* Thuan
Get the proc at index 0, then shift the rest to the left;
*/
struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        /* Highest priority : index 0 */
        if (empty(q))
        {
                return NULL;
        }
        struct pcb_t *result = q->proc[0];
        for (int i = 1; i < q->size; i++)
        {
                q->proc[i - 1] = q->proc[i]; // shift to the left
        }

        q->size--;
        q->proc[q->size] = NULL;

        return result;
}
