#include "http_server.h"
#include "../external/include/owl/collections/hashmap.h"
#include "../external/include/owl/systems/thread_pool.h"
#include "../external/include/owl/utils/log.h"
#include "http_req.h"
#include "server.h"
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROTOCOL 0
#define PORT 8000
#define BACKLOG 10
#define MAX_BACKLOG 1000
#define THREADS 20

#define MAX_LINE 4096
#define MAX_RESPONSE_SIZE 2e+6 // 2 MB
#define CHUNK_SIZE 1e+6        // 1 MB

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

static void handle_clients(http_server_t *server);
char *bin2hex(const unsigned char *input, size_t len);
void err_n_die(char *fmt, ...);

typedef struct
{
    char *uri;
    char *(*handler)(http_req_t *req);
} http_route_t;

static int route_compare(const void *a, const void *b, void *udata)
{
    const http_route_t *ra = a;
    const http_route_t *rb = b;
    return strcmp(ra->uri, rb->uri);
}

static uint64_t route_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const http_route_t *route = item;
    return owl_hashmap_sip(route->uri, strlen(route->uri), seed0, seed1);
}

http_server_t *http_server_init(int port, int backlog)
{
    if (!backlog || backlog < 10) backlog = BACKLOG;
    if (backlog > MAX_BACKLOG) backlog = MAX_BACKLOG;

    http_server_t *http_server = malloc(sizeof(http_server_t));
    http_server->server = server_init(AF_INET, SOCK_STREAM, PROTOCOL, INADDR_ANY,
                                      port, backlog);
    http_server->routes = owl_hashmap_new(sizeof(http_route_t), sizeof(http_route_t), 0, 0,
                                          route_hash, route_compare, NULL, NULL);

    return http_server;
}

// This will be passed to the thread function
typedef struct
{
    int conn_fd;
    http_server_t *server;
} client_server_t;

void *client_handler(void *arg);

static void handle_clients(http_server_t *http_server)
{
    int addrlen = sizeof(http_server->server->address);
    owl_thread_pool_t *tp = owl_thread_pool_init(THREADS);

    owl_println("[🐙] Server started listening on port %d", PORT);

    for (;;)
    {
        owl_println("[🐙] Waiting for connections ...");
        fflush(stdout);

        client_server_t *client_server = malloc(sizeof(client_server_t));
        client_server->server = http_server;

        client_server->conn_fd = accept(http_server->server->socket_fd,
                                        (struct sockaddr *)&http_server->server->address,
                                        (socklen_t *)&addrlen);

        owl_worker_task_t handle_client = owl_worker_task_init(client_handler, client_server);
        owl_thread_pool_enqueue_task(tp, &handle_client);
    }

    owl_thread_pool_free(tp);
}

static char *handle_404()
{
    return "";
}

// thread function
void *client_handler(void *arg)
{
    client_server_t *client_server = arg;
    uint8_t recv_line[MAX_LINE + 1];
    uint8_t buff[MAX_LINE + 1];
    uint8_t req_string[3000];
    char *hello = "HTTP/1.1 200 OK\r\n\r\n<html><head><title>Kraken Server</title></head><body><h2 style=\"color:lightcoral;\">404 Not found!!</h2></body><html>";
    int n;
    size_t req_len = 0;

    memset(recv_line, 0, MAX_LINE);

    // print the request
    while ((n = recv(client_server->conn_fd, recv_line, MAX_LINE - 1, 0)) > 0)
    {
        fprintf(stdout, "\n%s\n\n%s", bin2hex(recv_line, n), recv_line);

        memcpy(req_string + req_len, recv_line, n);
        req_len += n;

        if (recv_line[n - 1] == '\n') break;

        memset(recv_line, 0, MAX_LINE);
    }

    if (n == 0)
        owl_println("Connection closed by client");
    else if (n < 0)
        err_n_die("read error");

    // get the http headers
    http_req_t *http_req = http_req_init((char *)req_string);
    owl_hashmap_t *routes_map = client_server->server->routes;

    http_route_t *route = owl_hashmap_get(routes_map, &(http_route_t){.uri = http_req->uri});

    if (route)
    {
        char *response = route->handler(http_req);
        size_t response_len = strlen(response);

        // handle if the response size is too large
        if (response_len > MAX_RESPONSE_SIZE)
        {
            owl_println("[🐙] Response too large, sending in chunks...");

            size_t offset = 0;
            while (offset < response_len)
            {
                size_t chunk_size = MIN(CHUNK_SIZE, response_len - offset);

                if (send(client_server->conn_fd, response + offset, chunk_size, 0) < 0)
                    err_n_die("Error while sending");

                offset += chunk_size;
            }
        }
        else
        {
            snprintf((char *)buff, sizeof(buff), "%s", response);
            if (send(client_server->conn_fd, (char *)buff, strlen((char *)buff), 0) < 0)
                err_n_die("Error while sending");
        }
    }
    else
    {
        snprintf((char *)buff, sizeof(buff), "%s", hello);
        if (send(client_server->conn_fd, (char *)buff, strlen((char *)buff), 0) < 0)
            err_n_die("Error while sending");
    }

    close(client_server->conn_fd);
    http_req_free(http_req);
    free(client_server);

    return NULL;
}

char *bin2hex(const unsigned char *input, size_t len)
{
    char *result;
    char *hexits = "0123456789ABCDEF";

    if (input == NULL || len <= 0) return NULL;

    // (2 hexits+space)/chr + NULL
    int resultlength = (len * 3) + 1;

    result = malloc(resultlength);
    bzero(result, resultlength);

    for (int i = 0; i < len; i++)
    {
        result[i * 3] = hexits[input[i] >> 4];
        result[(i * 3) + 1] = hexits[input[i] & 0x0F];
        result[(i * 3) + 2] = ' ';
    }

    return result;
}

void err_n_die(char *fmt, ...)
{
    int errno_save;
    va_list ap;

    errno_save = errno;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    if (errno_save != 0)
    {
        fprintf(stderr, "(errno = %d) : %s\n", errno_save,
                gai_strerror(errno_save));
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    va_end(ap);
    exit(0);
}

void http_server_listen(http_server_t *server)
{
    handle_clients(server);
}

int register_route(http_server_t *server, char *uri, char *(*handler)(http_req_t *req))
{
    if (owl_hashmap_oom(server->routes)) return -1;

    owl_hashmap_set(server->routes, &(http_route_t){.uri = uri, .handler = handler});

    return 0;
}

void http_server_free(http_server_t *server)
{
    owl_hashmap_free(server->routes);
    server_free(server->server);
    free(server);
}