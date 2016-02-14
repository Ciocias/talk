#ifndef TLK_MSG_QUEUE_H
#define TLK_MSG_QUEUE_H

#include "tlk_semaphores.h"
#include "tlk_users.h"

typedef struct _tlk_queue_s {

  int read_index;
  int write_index;

  tlk_sem_t empty_count;
  tlk_sem_t fill_count;
  tlk_sem_t read_mutex;
  tlk_sem_t write_mutex;

  unsigned int buffer_length;
  tlk_message_t *buffer;

} tlk_queue_t;

/*
 * Initialize a new tlk_queue_t with @size
 * Returns a pointer to the new structure on success, NULL on failure
 */
tlk_queue_t *tlk_queue_new (int size);

/*
 * Enqueue @msg in @q, this function is thread-safe
 * Returns 0 on success, -1 on failure
 */
int tlk_queue_enqueue (tlk_queue_t *q, const tlk_message_t *msg);

/*
 * Dequeue first message in @q and put it inside @msg
 * Returns 0 on success, -1 on failure
 */
int tlk_queue_dequeue (tlk_queue_t *q, tlk_message_t *msg);

/*
 * Destroy queue @q
 * Returns 0 on success, -1 on failure
 */
int tlk_queue_free (tlk_queue_t *q);

#endif
