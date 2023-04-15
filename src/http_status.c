#include "http_status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_STATUS_CODE_LEN 3

static uint64_t status_code_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const status_code_with_reason_t *sc = item;
    return owl_hashmap_sip(&sc->code, HTTP_STATUS_CODE_LEN, seed0, seed1);
}

static int status_code_compare(const void *a, const void *b, void *udata)
{
    const status_code_with_reason_t *ra = a;
    const status_code_with_reason_t *rb = b;
    return ra->code == rb->code;
}

owl_hashmap_t *http_status_map_init()
{
    owl_hashmap_t *http_status_map = owl_hashmap_new(sizeof(status_code_with_reason_t), 15, 0, 0,
                                                     status_code_hash, status_code_compare, NULL, NULL);

    if (!http_status_map)
    {
        fprintf(stderr, "Failed to create HTTP status map\n");
        exit(1);
    }

    status_code_with_reason_t statuses[] = {
        {100, "CONTINUE"},
        {200, "OK"},
        {201, "CREATED"},
        {202, "ACCEPTED"},
        {204, "NO_CONTENT"},
        {400, "BAD_REQUEST"},
        {401, "UNAUTHORIZED"},
        {403, "FORBIDDEN"},
        {404, "NOT_FOUND"},
        {405, "METHOD_NOT_ALLOWED"},
        {500, "INTERNAL_SERVER_ERROR"},
        {501, "NOT_IMPLEMENTED"},
        {502, "BAD_GATEWAY"},
        {503, "SERVICE_UNAVAILABLE"},
        {504, "GATEWAY_TIMEOUT"}};

    const size_t num_statuses = sizeof(statuses) / sizeof(status_code_with_reason_t);

    for (size_t i = 0; i < num_statuses; i++)
    {
        owl_hashmap_set(http_status_map, &statuses[i]);
    }

    return http_status_map;
}

void http_status_map_free(owl_hashmap_t *map)
{
    owl_hashmap_free(map);
}