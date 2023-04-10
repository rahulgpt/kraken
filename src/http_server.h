#ifndef http_server_h
#define http_server_h

#include "../external/include/owl/collections/hashmap.h"
#include "http_req.h"
#include "server.h"

typedef struct http_server
{
    server_t *server;
    owl_hashmap_t *routes;
} http_server_t;

http_server_t *http_server_init(int port, int backlog);
int register_route(http_server_t *server, char *uri, char *(*handler)(http_req_t *req));

/*
 * Start listening for incoming connections
 */
void http_server_listen(http_server_t *server);
void http_server_free(http_server_t *server);

#endif /* http_server_h */