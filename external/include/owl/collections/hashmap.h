#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct hashmap owl_hashmap_t;

owl_hashmap_t *owl_hashmap_new(size_t elsize, size_t cap,
                               uint64_t seed0, uint64_t seed1,
                               uint64_t (*hash)(const void *item,
                                                uint64_t seed0, uint64_t seed1),
                               int (*compare)(const void *a, const void *b,
                                              void *udata),
                               void (*elfree)(void *item),
                               void *udata);

owl_hashmap_t *owl_hashmap_new_with_allocator(
    void *(*malloc)(size_t),
    void *(*realloc)(void *, size_t),
    void (*free)(void *),
    size_t elsize, size_t cap,
    uint64_t seed0, uint64_t seed1,
    uint64_t (*hash)(const void *item,
                     uint64_t seed0, uint64_t seed1),
    int (*compare)(const void *a, const void *b,
                   void *udata),
    void (*elfree)(void *item),
    void *udata);

void owl_hashmap_free(owl_hashmap_t *map);
void owl_hashmap_clear(owl_hashmap_t *map, bool update_cap);
void *owl_hashmap_get(owl_hashmap_t *map, const void *item);
void *owl_hashmap_set(owl_hashmap_t *map, const void *item);
void *owl_hashmap_delete(owl_hashmap_t *map, void *item);
bool owl_hashmap_scan(owl_hashmap_t *map,
                      bool (*iter)(const void *item, void *udata), void *udata);
bool owl_hashmap_iter(owl_hashmap_t *map, size_t *i, void **item);

// helpers

uint64_t owl_hashmap_sip(const void *data, size_t len,
                         uint64_t seed0, uint64_t seed1);
uint64_t owl_hashmap_murmur(const void *data, size_t len,
                            uint64_t seed0, uint64_t seed1);

// Getters

size_t owl_hashmap_count(owl_hashmap_t *map);
bool owl_hashmap_oom(owl_hashmap_t *map);

#endif