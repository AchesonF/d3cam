#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

void queue_reset (struct queue_t *pQ)
{
	memset(pQ->data, 0, SIZE_BUFF_QUEUE);
	pQ->size = 0;
	pQ->front = 0;
	pQ->rear = 0;
}

struct queue_t *queue_init (void)
{
	struct queue_t *pQ = malloc(sizeof(struct queue_t));
	if (pQ == NULL) {
		printf("ERROR : queue malloc");

		return NULL;
	}

	queue_reset(pQ);

	return pQ;
}

void queue_destroy (struct queue_t *pQ)
{
	if (pQ) {
		free(pQ);
	}
}

int queue_isFull (struct queue_t *pQ)
{
	if (pQ->size + 1 > SIZE_BUFF_QUEUE) {
		return 1;
	}

	return 0;
}

int queue_isEmpty (struct queue_t *pQ)
{
	if (pQ->size == 0) {
		return 1;
	}

	return 0;
}

uint8_t queue_peek (struct queue_t *pQ, int index)
{
	if (pQ->size < index) {
		return -1;
	}

	return (pQ->data[(pQ->rear + index) % SIZE_BUFF_QUEUE]);
}

int queue_output (struct queue_t *pQ, void *buffer, int size)
{
	if (pQ->size - size < 0) {
		return -1;
	}

	if ((buffer == NULL) && (size != 0)) {
		pQ->rear = (pQ->rear + size) % SIZE_BUFF_QUEUE;
		pQ->size -= size;

		return 0;
	}

	uint8_t *pb = (uint8_t *) buffer;
	int i = 0;

	for (i = 0; i < size; i++) {
        pb[i] = pQ->data[pQ->rear];
        pQ->rear = (pQ->rear + 1) % SIZE_BUFF_QUEUE;
    }

    pQ->size -= size;

    return 0;
}

int queue_input (struct queue_t *pQ, void *buffer, int size)
{
	if (pQ->size + size > SIZE_BUFF_QUEUE) {
		return -1;
	}

	uint8_t *pb = (uint8_t *) buffer;
	int i = 0;

	for (i = 0; i < size; i++) {
		pQ->data[pQ->front] = pb[i];
		pQ->front = (pQ->front + 1) % SIZE_BUFF_QUEUE;
	}

	pQ->size += size;

	return 0;
}

void queue_copy (struct queue_t *pQ, int start, int size, uint8_t *buffer)
{
	int i = 0;

	for (i = 0; i < size; i++) {
		buffer[i] = pQ->data[(pQ->rear + start + i) % SIZE_BUFF_QUEUE];
	}
}

#ifdef __cplusplus
}
#endif


