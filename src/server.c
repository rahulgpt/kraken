#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct server server_t;

server_t *server_init(int domain, int service, int protocol, u_long interface, int port, int backlog)
{
    server_t *server = malloc(sizeof(server_t));
    server->domain = domain;
    server->service = service;
    server->protocol = protocol;
    server->interface = interface;
    server->port = port;
    server->backlog = backlog;

    memset(&server->address, 0, sizeof(server->address));
    server->address.sin_family = domain;
    server->address.sin_port = htons(port);
    server->address.sin_addr.s_addr = htonl(interface);

    server->socket_fd = socket(domain, service, protocol);
    if (server->socket_fd < 0)
    {
        perror("Failed to create socket...\n");
        exit(1);
    }

    // allow restarting the server right away, without waiting for
    // connections from the previous run to leave the TIME_WAIT state
    int reuse = 1;
    setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if ((bind(server->socket_fd, (struct sockaddr *)&server->address, sizeof(server->address))) < 0)
    {
        perror("Failed to bind the socket...\n");
        exit(1);
    };

    if ((listen(server->socket_fd, server->backlog)) < 0)
    {
        perror("Failed to start listening for connections...\n");
        exit(1);
    };

    return server;
}

void server_free(server_t *server)
{
    free(server);
}