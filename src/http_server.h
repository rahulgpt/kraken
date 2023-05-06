#ifndef http_server_h
#define http_server_h

#include "../external/include/owl/collections/hashmap.h"
#include "../external/include/owl/systems/thread_pool.h"
#include "http_req.h"
#include "http_res.h"
#include "server.h"

#define MAX_STATIC_FILES_PATH 10

typedef struct http_server
{
    server_t *server;
    int port;
    owl_hashmap_t *routes;
    owl_hashmap_t *http_status_map;
    owl_thread_pool_t *thread_pool;
    char *static_files_path[MAX_STATIC_FILES_PATH];
    short int num_registered_file_paths;
} http_server_t;

http_server_t *http_server_init(int port, int backlog);
int register_route(http_server_t *server, char *uri, char *(*handler)(http_req_t *req, http_res_t *res));
void register_static(http_server_t *server, char *path);

/*
 * Start listening for incoming connections
 */
void http_server_listen(http_server_t *server);
void http_server_free(http_server_t *server);

#endif /* http_server_h */