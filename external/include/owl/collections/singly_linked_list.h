#ifndef owl_singly_linked_list_h
#define owl_singly_linked_list_h

#include <stdlib.h>

typedef struct owl__Node
{
    unsigned int id;
    void *data;
    struct owl__Node *next;
} owl_sll_node_t;

typedef struct __opaq_owl__sll owl_sll_t;

owl_sll_t *owl_sll_init(size_t el_size, void (*el_free)(void *data));
void owl_sll_free(owl_sll_t *list);
void owl_sll_binsert(owl_sll_t *list, void *data);
void *owl_sll_bremove(owl_sll_t *list);
void owl_sll_finsert(owl_sll_t *list, void *data);
void *owl_sll_fremove(owl_sll_t *list);
void owl_sll_print(owl_sll_t *list, void (*format)(void *data), char *connection_symbol);

// Getters

owl_sll_node_t *owl_sll_head(owl_sll_t *list);
owl_sll_node_t *owl_sll_tail(owl_sll_t *list);
size_t owl_sll_length(owl_sll_t *list);

#endif /* owl_singly_linked_list_h */