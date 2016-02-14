#include "../../include/tlk_msg_queue.h"

#include <stdlib.h>

/*
 * Initialize a new tlk_queue_t with @size
 * Returns a pointer to the new structure on success, NULL on failure
 */
tlk_queue_t *tlk_queue_new (int size) {

	int ret = 0;
	tlk_queue_t *aux = (tlk_queue_t *) malloc(sizeof(tlk_queue_t));

  ret = tlk_sem_init(&(aux -> empty_count), size, size);
  if (ret) return NULL;

	ret = tlk_sem_init(&(aux -> fill_count), 0, size);
  if (ret) return NULL;

	ret = tlk_sem_init(&(aux -> read_mutex), 1, 1);
  if (ret) return NULL;

	ret = tlk_sem_init(&(aux -> write_mutex), 1, 1);
  if (ret) return NULL;

  aux -> buffer = (tlk_message_t *) malloc(size * sizeof(tlk_message_t ));
  aux -> buffer_length = size;

  aux -> read_index = 0;
  aux -> write_index = 0;

  return aux;
}

/*
 * Enqueue @msg in @q, this function is thread-safe
 * Returns 0 on success, -1 on failure
 */
int tlk_queue_enqueue (tlk_queue_t *q, const tlk_message_t *msg) {

  int ret = 0;

  ret = tlk_sem_wait(&(q -> empty_count));
  if (ret) return ret;

  ret = tlk_sem_wait(&(q -> write_mutex));
  if (ret) return ret;

  /* Critical Section */
  (q -> buffer)[q -> write_index] =  *msg;
  q -> write_index = ((q -> write_index) + 1) % (q -> buffer_length);

  ret = tlk_sem_post(&(q -> write_mutex));
  if (ret) return ret;

  ret = tlk_sem_post(&(q -> fill_count));
  if (ret) return ret;

  return ret;
}

/*
 * Dequeue first message in @q and put it inside @msg
 * Returns 0 on success, -1 on failure
 */
int tlk_queue_dequeue (tlk_queue_t *q, tlk_message_t *msg) {

  int ret = 0;

  ret = tlk_sem_wait(&(q -> fill_count));
  if (ret) return ret;

	ret = tlk_sem_wait(&(q -> read_mutex));
  if (ret) return ret;

  /* Critical Section */
	*msg = (q -> buffer)[q -> read_index];
  q -> read_index = ((q -> read_index) + 1) % (q -> buffer_length);

  ret = tlk_sem_post(&(q -> read_mutex));
  if (ret) return ret;

	ret = tlk_sem_post(&(q -> empty_count));
  if (ret) return ret;

	return ret;
}

/*
 * Destroy queue @q
 * Returns 0 on success, -1 on failure
 */
int tlk_queue_free (tlk_queue_t *q) {

  int ret = 0;

  /* Delete struct semaphores */
  ret = tlk_sem_destroy(&(q -> empty_count));
  if (ret) return ret;

  ret = tlk_sem_destroy(&(q -> fill_count));
  if (ret) return ret;

  ret = tlk_sem_destroy(&(q -> read_mutex));
  if (ret) return ret;

  ret = tlk_sem_destroy(&(q -> write_mutex));
  if (ret) return ret;

  /* Free memory */
  free(q -> buffer);
  free(q);

  return ret;
}
