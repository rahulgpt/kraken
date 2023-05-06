#include "http_req.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum HTTPMethod select_method(char *method)
{
    if (strcmp(method, "GET") == 0) return GET;
    if (strcmp(method, "POST") == 0) return POST;
    if (strcmp(method, "PUT") == 0) return PUT;
    if (strcmp(method, "HEAD") == 0) return HEAD;
    if (strcmp(method, "PATCH") == 0) return PATCH;
    if (strcmp(method, "DELETE") == 0) return DELETE;
    if (strcmp(method, "CONNECT") == 0) return CONNECT;
    if (strcmp(method, "OPTIONS") == 0) return OPTIONS;
    if (strcmp(method, "TRACE") == 0) return TRACE;

    // Handle this properly. Skip the connection if not supported
    // or respond appropriatly
    perror("Http method is not supported...\n");
    exit(1);
}

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

bool header_iter(const void *item, void *udata)
{
    const header_t *header = item;
    printf("name: %s, value: %s\n", header->name, header->value);
    return true;
}

static owl_hashmap_t *parse_http_headers(char *header_fields)
{
    owl_hashmap_t *header_map = owl_hashmap_new(sizeof(header_t), sizeof(header_t), 0, 0,
                                                header_hash, header_compare, NULL, NULL);

    char *header_pos = NULL;
    char *header = strtok_r(header_fields, "\n", &header_pos);
    while (header != NULL)
    {
        char *name = strtok(header, ":");
        char *value = strtok(NULL, "\n");
        if (value != NULL) value++; // remove leading space

        owl_hashmap_set(header_map, &(header_t){.name = name, .value = value});
        header = strtok_r(NULL, "\n", &header_pos);
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

    req->method = select_method(method);
    req->uri = malloc(strlen(uri) + 1);
    strcpy(req->uri, uri);
    req->http_version = (float)atof(http_version);
    req->headers = parse_http_headers(header_fields);

    return req;
}

void http_req_free(http_req_t *http_req)
{
    owl_hashmap_free(http_req->headers);
    free(http_req->uri);
    free(http_req);
}