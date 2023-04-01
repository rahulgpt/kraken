#include "http_req.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum HTTPMethod get_method(char *method)
{
    if (strcmp(method, "GET") == 0) return GET;
    if (strcmp(method, "POST") == 0) return POST;

    // Handle this properly. Skip the connection if not supported
    // or respond appropriatly
    perror("Http method is not supported...\n");
    exit(1);
}

typedef struct
{
    char *name;
    char *value;
} header_t;

static int header_compare(const void *a, const void *b, void *udata)
{
    const header_t *ha = a;
    const header_t *hb = b;
    return strcmp(ha->name, hb->name);
}

static uint64_t header_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const header_t *header = item;
    return owl_hashmap_sip(header->name, strlen(header->name), seed0, seed1);
}

static owl_hashmap_t *parse_http_headers(char *header_fields)
{
    owl_hashmap_t *header_map = owl_hashmap_new(sizeof(header_t), sizeof(header_t), 0, 0,
                                                header_hash, header_compare, NULL, NULL);

    char *header = strtok(header_fields, "\n");
    while (header != NULL)
    {
        char *name = strtok(header, ":");
        char *value = strtok(NULL, "\n");
        value++; // remove leading space

        owl_hashmap_set(header_map, &(header_t){.name = name, .value = value});
        header = strtok(NULL, "\n");
    }

    return header_map;
}

http_req_t *http_req_init(char *req_string)
{
    http_req_t *req = malloc(sizeof(http_req_t));

    // cannot mutate a string literal
    char requested[strlen(req_string)];
    strcpy(requested, req_string);

    for (int i = 0; i < strlen(requested) - 1; i++)
    {
        if (requested[i] == '\n' && requested[i + 1] == '\n')
        {
            requested[i + 1] = '|';
        }
    }

    char *req_line = strtok(requested, "\n");
    char *header_fields = strtok(NULL, "|");
    char *body = strtok(NULL, "|");

    char *method = strtok(req_line, " ");
    char *uri = strtok(NULL, " ");
    char *http_version = strtok(NULL, " ");
    http_version = strtok(http_version, "/");
    http_version = strtok(NULL, "/");

    req->method = get_method(method);
    req->uri = uri;
    req->http_version = (float)atof(http_version);
    req->headers = parse_http_headers(header_fields);

    return req;
}
