#ifndef http_req_h
#define http_req_h

#include "../external/include/owl/collections/hashmap.h"
#include <stddef.h>

// This module can be responsible for parsing out the http request

typedef struct
{
    char *name;
    char *value;
} header_t;

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
    char *uri;   // request target exactly as sent, e.g. "/hello?name=kraken"
    char *path;  // uri without the query string, url decoded. Used for routing
    char *query; // raw query string after '?', NULL if there is none
    float http_version;
    owl_hashmap_t *headers;      // header names are stored lowercased
    owl_hashmap_t *query_params; // decoded key/value pairs from the query string
    char *body;                  // NULL terminated, but may also contain NULL bytes
    size_t body_len;
    char *raw; // the whole request as it was read from the socket
    size_t raw_len;
    char remote_addr[64]; // ip address of the peer that opened the connection
    int remote_port;

    // backing storage for the strings above, owned by the request
    char *head_buf;
    char *query_buf;
} http_req_t;

http_req_t *http_req_init(const char *raw, size_t raw_len);
void http_req_free(http_req_t *http_req);

/*
 * Get a request header by name (case insensitive). Returns NULL if missing.
 */
char *req_header(http_req_t *req, const char *name);

/*
 * Get a query string parameter, e.g. req_query_param(req, "name") for
 * "/hello?name=kraken". Returns NULL if missing.
 */
char *req_query_param(http_req_t *req, const char *name);

/*
 * The request method as a string, e.g. "GET"
 */
const char *req_method(http_req_t *req);

#endif /* http_req_h */
