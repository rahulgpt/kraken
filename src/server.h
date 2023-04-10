#ifndef server_h
#define server_h

#include <netinet/in.h>
#include <sys/socket.h>

// This module can be responsible for creating a socket and start listening

typedef struct server
{
    int domain;
    int service;
    int protocol;
    u_long interface;
    int port;
    int backlog;
    int socket_fd;
    struct sockaddr_in address;
} server_t;

server_t *server_init(int domain, int service, int protocol, u_long interface,
                      int port, int backlog);

void server_free(server_t *server);

#endif /* server_h */