#ifndef http_status_h
#define http_status_h

#include "../external/include/owl/collections/hashmap.h"

typedef struct status_code_with_reason
{
    int code;
    char *reason;
} status_code_with_reason_t;

owl_hashmap_t *http_status_map_init();
void http_status_map_free(owl_hashmap_t *map);

#endif /* http_status_h */