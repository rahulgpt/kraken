#include "http_server.h"
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

#define MAX_LINE 4096

static void handle_clients(server_t *server);
char *bin2hex(const unsigned char *input, size_t len);
void err_n_die(char *fmt, ...);

http_server_t *http_server_init(int port, int backlog)
{
    if (!backlog || backlog < 10) backlog = BACKLOG;
    if (backlog > MAX_BACKLOG) backlog = MAX_BACKLOG;

    http_server_t *http_server = malloc(sizeof(http_server_t));
    http_server->server = server_init(AF_INET, SOCK_STREAM, PROTOCOL, INADDR_ANY,
                                      port, backlog, handle_clients);

    return http_server;
}

static void handle_clients(server_t *server)
{
    uint8_t recv_line[MAX_LINE + 1];
    uint8_t buff[MAX_LINE + 1];
    uint8_t req_string[3000];
    char *hello = "HTTP/1.1 200 OK\r\n\r\n<html><head><title>Kraken Server</title></head><body><h2 style=\"color:lightcoral;\">Hello from Kraken</h2></body><html>";
    int addrlen = sizeof(server->address);
    int conn_fd, n;
    size_t req_len = 0;

    owl_println("[🐙] Server started listening on port %d", PORT);

    for (;;)
    {
        owl_println("[🐙] Waiting for connections ...");
        fflush(stdout);
        conn_fd = accept(server->socket_fd, (struct sockaddr *)&server->address,
                         (socklen_t *)&addrlen);

        memset(recv_line, 0, MAX_LINE);

        while ((n = recv(conn_fd, recv_line, MAX_LINE - 1, 0)) > 0)
        {
            fprintf(stdout, "\n%s\n\n%s", bin2hex(recv_line, n), recv_line);

            memcpy(req_string + req_len, recv_line, n);
            req_len += n;

            if (recv_line[n - 1] == '\n') break;

            memset(recv_line, 0, MAX_LINE);
        }

        http_req_init((char *)req_string);

        if (n < 0) err_n_die("read error");

        // owl_println("%s", buffer);
        snprintf((char *)buff, sizeof(buff), "%s", hello);

        // write(conn_fd, hello, strlen(hello));
        send(conn_fd, (char *)buff, strlen((char *)buff), 0);
        close(conn_fd);
        owl_println("[🐙] Connection closed");
    }
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
    server_handle_connections(server->server);
}