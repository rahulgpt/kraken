#include "owl/utils/log.h"
#include "src/http_req.h"
#include "src/http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROTOCOL 0
#define PORT 8000
#define BACKLOG 10

char *home_handler(http_req_t *req)
{
    owl_println("Method: %d", req->method);
    owl_println("Home Handler called!!");
    return "HTTP/1.1 200 OK\r\n\r\n<html><head><title>Kraken Server</title></head><body><h2 style=\"color:lightcoral;\">Home Page</h2></body><html>";
}

char *about_handler(http_req_t *req)
{
    owl_println("Method: %d", req->method);
    owl_println("About Handler called!!");
    return "HTTP/1.1 200 OK\r\n\r\n<html><head><title>Kraken Server</title></head><body><h2 style=\"color:lightcoral;\">About Page</h2></body><html>";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/home", home_handler);
    register_route(server, "/about", about_handler);

    http_server_listen(server);
    http_server_free(server);
}