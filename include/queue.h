#ifndef __QUEUE_H__
#define __QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_BUFF_QUEUE		(8192)

struct queue_t {
	uint32_t		front;
	uint32_t		rear;
	int				size;

	uint8_t			data[SIZE_BUFF_QUEUE];
};

extern void queue_reset (struct queue_t *pQ);

extern struct queue_t *queue_init (void);

extern void queue_destroy (struct queue_t *pQ);

extern int queue_isFull (struct queue_t *pQ);

extern int queue_isEmpty (struct queue_t *pQ);

extern uint8_t queue_peek (struct queue_t *pQ, int index);

extern int queue_output (struct queue_t *pQ, void *buffer, int size);

extern int queue_input (struct queue_t *pQ, void *buffer, int size);

extern void queue_copy (struct queue_t *pQ, int start, int size, uint8_t *buffer);



#ifdef __cplusplus
}
#endif

#endif /* __QUEUE_H__ */



