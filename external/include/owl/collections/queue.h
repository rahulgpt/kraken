#ifndef owl_queue_h
#define owl_queue_h

#include "singly_linked_list.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct __opaq_owl__queue owl_queue_t;

owl_queue_t *owl_queue_init(size_t el_size, size_t capacity, void (*el_free)(void *data));
void owl_queue_free(owl_queue_t *queue);
int owl_queue_enqueue(owl_queue_t *queue, void *data);
void *owl_queue_dequeue(owl_queue_t *queue);
bool owl_queue_is_empty(owl_queue_t *queue);
bool owl_queue_is_full(owl_queue_t *queue);
void owl_queue_print(owl_queue_t *queue, void (*format)(void *data), char *connection_sym);

// Getters

size_t owl_queue_max_capacity(owl_queue_t *queue);
size_t owl_queue_n_items(owl_queue_t *queue);

#endif /* owl_queue_h */
