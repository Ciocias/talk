/* TODO: error checking */

#ifndef TLK_MSG_QUEUE_H
#define TLK_MSG_QUEUE_H

#include "tlk_min_heap.h"
#include "tlk_semaphores.h"
#include "tlk_users.h"

#include <stdlib.h>

typedef struct _tlk_queue_s {
  int last_key_value;
  tlk_sem_t empty_count;
  tlk_sem_t fill_count;
  tlk_sem_t read_mutex;
  tlk_sem_t write_mutex;
  min_heap *queue;
} tlk_queue_t;

/*
 * Initialize a new tlk_queue_t with @size
 * Returns a pointer to the newly created structure
 */
tlk_queue_t *tlk_queue_new (int size);

/*
 * Enqueue @msg in @q, this function is thread-safe
 * Returns 0 on success, propagates the errors on fail
 */
int tlk_queue_enqueue (tlk_queue_t *q, const tlk_message_t *msg);

/*
 * Dequeue first message in @q and put it inside @msg
 * Returns 0 on success, propagates the errors on fail
 */
int tlk_queue_dequeue (tlk_queue_t *q, tlk_message_t *msg);

/*
 * Free memory for queue @q
 * Returns nothing
 */
void tlk_queue_free (tlk_queue_t *q);

#endif
