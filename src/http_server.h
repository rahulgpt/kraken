#ifndef http_server_h
#define http_server_h

#include "../external/include/owl/collections/hashmap.h"
#include "http_req.h"
#include "server.h"

typedef struct
{
    server_t *server;
    owl_hashmap_t *routes;
} http_server_t;

http_server_t *http_server_init(int port, int backlog);
int register_route(server_t *server,
                   char *(*route_function)(http_server_t *server,
                                           http_req_t *request,
                                           char *uri,
                                           int num_methods, ...));

/*
 * Start listening for incoming connections
 */
void http_server_listen(http_server_t *server);
void http_server_free(http_server_t *server);

#endif /* http_server_h */