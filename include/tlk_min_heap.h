#ifndef MIN_HEAP_H
#define	MIN_HEAP_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MIN_HEAP_DATA_TYPE void *
#define MIN_HEAP_DATA_TYPE_PRINTF "%s"
#define MIN_HEAP_MAX_SIZE 100

typedef void min_heap;

typedef struct _min_heap_struct_entry {
    int index;
    int key;
    MIN_HEAP_DATA_TYPE value;
} min_heap_struct_entry;

extern min_heap * min_heap_new();
extern min_heap_struct_entry * min_heap_min(min_heap * mh);
extern min_heap_struct_entry * min_heap_insert(min_heap * mh, int p, MIN_HEAP_DATA_TYPE value);
extern int min_heap_is_empty(min_heap * mh);
extern min_heap_struct_entry * min_heap_remove_min(min_heap * mh);
extern int min_heap_size(min_heap *mh);
extern void heapsort(int* v, int size);

extern min_heap_struct_entry * min_heap_replace_value(min_heap * mh, min_heap_struct_entry * e, MIN_HEAP_DATA_TYPE value);
extern min_heap_struct_entry * min_heap_replace_key(min_heap * mh, min_heap_struct_entry * e, int key);
extern min_heap_struct_entry * min_heap_remove(min_heap * mh, min_heap_struct_entry * e);

extern void min_heap_free(min_heap *mh);

#ifdef	__cplusplus
}
#endif

#endif	/* MIN_HEAP_H */
