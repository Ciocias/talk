#include "../../include/tlk_msg_queue.h"

/*
 * Initialize a new tlk_queue_t with @size
 * Returns a pointer to the newly created structure
 */
tlk_queue_t *tlk_queue_new (int size) {
	int ret;
	tlk_queue_t *aux = (tlk_queue_t *) malloc(sizeof(tlk_queue_t));

  ret = tlk_sem_init(&(aux -> empty_count), size, size);

	ret = tlk_sem_init(&(aux -> fill_count), 0, size);

	ret = tlk_sem_init(&(aux -> read_mutex), 1, 1);

	ret = tlk_sem_init(&(aux -> write_mutex), 1, 1);

  aux -> buffer = (tlk_message_t *) malloc(size * sizeof(tlk_message_t ));
  aux -> buffer_length = size;
  aux -> read_index = 0;
  aux -> write_index = 0;

  return aux;
}

/*
 * Enqueue @msg in @q, this function is thread-safe
 * Returns 0 on success, propagates the errors on fail
 */
int tlk_queue_enqueue (tlk_queue_t *q, const tlk_message_t *msg) {
  int ret;

  ret = tlk_sem_wait(&(q -> empty_count));

  ret = tlk_sem_wait(&(q -> write_mutex));

  (q -> buffer)[q -> write_index] =  *msg;
  q -> write_index = ((q -> write_index) + 1) % (q -> buffer_length);

  ret = tlk_sem_post(&(q -> write_mutex));

  ret = tlk_sem_post(&(q -> fill_count));

  return ret;
}

/*
 * Dequeue first message in @q and put it inside @msg
 * Returns 0 on success, propagates the errors on fail
 */
int tlk_queue_dequeue (tlk_queue_t *q, tlk_message_t *msg) {
  int ret;

  ret = tlk_sem_wait(&(q -> fill_count));

	ret = tlk_sem_wait(&(q -> read_mutex));

	*msg = (q -> buffer)[q -> read_index];
  q -> read_index = ((q -> read_index) + 1) % (q -> buffer_length);

  ret = tlk_sem_post(&(q -> read_mutex));

	ret = tlk_sem_post(&(q -> empty_count));

	return ret;
}

/*
 * Free memory for queue @q
 * Returns nothing
 */
void tlk_queue_free (tlk_queue_t *q) {

  tlk_sem_destroy(&(q -> empty_count));
  tlk_sem_destroy(&(q -> fill_count));
  tlk_sem_destroy(&(q -> read_mutex));
  tlk_sem_destroy(&(q -> write_mutex));

  free(q -> buffer);

  free(q);

}
