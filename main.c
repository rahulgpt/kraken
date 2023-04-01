#include "owl/utils/log.h"
#include "src/http_req.h"
#include "src/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROTOCOL 0
#define PORT 80
#define BACKLOG 10

void launch(server_t *server)
{
    char buffer[30000];
    char *hello = "HTTP/1.1 / 200 OK\r\n\r\n<html><head><title>Kraken Server</title></head><body><h2>Hello from Kraken</h2></body><html>";
    int addrlen = sizeof(server->address);
    int new_socket;

    while (1)
    {

        printf("------------ Waiting for a new connection ----------\n");
        new_socket = accept(server->socket, (struct sockaddr *)&server->address,
                            (socklen_t *)&addrlen);
        read(new_socket, buffer, 30000);

        owl_println("%s", buffer);
        write(new_socket, hello, strlen(hello));
        close(new_socket);
    }
}

int main()
{
    server_t *server = server_init(AF_INET, SOCK_STREAM, PROTOCOL, INADDR_ANY, PORT, BACKLOG);
    launch(server);
    free(server);
}