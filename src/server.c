#include "server.h"
#include <stdio.h>
#include <stdlib.h>

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

    server->address.sin_family = domain;
    server->address.sin_port = htons(port);
    server->address.sin_addr.s_addr = htonl(interface);

    server->socket = socket(domain, service, protocol);
    if (server->socket == 0)
    {
        perror("Failed to connect socket...\n");
        exit(1);
    }

    if ((bind(server->socket, (struct sockaddr *)&server->address, sizeof(server->address))) < 0)
    {
        perror("Failed to bind the socket...\n");
        exit(1);
    };

    if ((listen(server->socket, server->backlog)) < 0)
    {
        perror("Failed to start listening for connections...\n");
        exit(1);
    };

    return server;
}