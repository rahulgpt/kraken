#ifndef http_req_h
#define http_req_h

#include "../external/include/owl/collections/hashmap.h"

// This module can be responsible for parsing out the http request

enum HTTPMethod
{
    GET,
    POST,
    PUT,
    HEAD,
    PATCH,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
};

typedef struct HTTPReq
{
    int method;
    char *uri;
    float http_version;
    owl_hashmap_t *headers;
    char *body;
} http_req_t;

http_req_t *http_req_init(char *req_string);
void http_req_free(http_req_t *http_req);

#endif /* http_req_h */