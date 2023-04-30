#include "http_server.h"
#include "../external/include/owl/collections/hashmap.h"
#include "../external/include/owl/systems/thread_pool.h"
#include "../external/include/owl/utils/log.h"
#include "http_req.h"
#include "http_status.h"
#include "server.h"
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PROTOCOL 0
#define PORT 8000
#define BACKLOG 10
#define MAX_BACKLOG 1000
#define THREADS 20
#define BUFF_SIZE 4096

#define MAX_LINE 4096
#define MAX_HEADER_LEN 1024
#define MAX_PATH_LEN 100

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void handle_clients(http_server_t *server);
char *bin2hex(const unsigned char *input, size_t len);
void err_n_die(char *fmt, ...);
void handle_interrupt(int signal_num);

static http_server_t *server;

typedef struct
{
    char *uri;
    char *(*handler)(http_req_t *req, http_res_t *res);
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

    http_server->http_status_map = http_status_map_init();
    http_server->num_registered_file_paths = 0;

    // bind the global server var to the latest instance
    server = http_server;

    signal(SIGINT, handle_interrupt);

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
    http_server->thread_pool = tp;

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
}

// thread function
void *client_handler(void *arg)
{
    client_server_t *client_server = arg;
    uint8_t recv_line[MAX_LINE + 1];
    uint8_t buff[MAX_LINE + 1];
    uint8_t req_string[3000];
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
    owl_hashmap_t *http_status_map = client_server->server->http_status_map;

    http_res_t *http_res = http_res_init();

    http_route_t *route = owl_hashmap_get(routes_map, &(http_route_t){.uri = http_req->uri});

    // Date header indicates the data and time the response was generated
    char date_str[100];
    time_t now = time(NULL);
    struct tm tm = *gmtime(&now);
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", &tm);

    if (route)
    {
        // After calling the handler, "http_res" will be populated
        char *res = route->handler(http_req, http_res);
        size_t res_len = strlen(res);

        // get the reason string from the status map
        status_code_with_reason_t *scr = owl_hashmap_get(http_status_map,
                                                         &(status_code_with_reason_t){.code = http_res->status_code});
        if (!scr) err_n_die("%d http status is not supported by the server.", http_res->status_code);
        http_res->reason = scr->reason;

        // format headers to send in the response
        char headers[MAX_HEADER_LEN] = "";
        size_t headers_len = 0;
        void *item;
        size_t iter = 0;
        while (owl_hashmap_iter(http_res->headers, &iter, &item))
        {
            const header_t *header = item;
            headers_len += snprintf(headers + headers_len, sizeof(headers) - headers_len,
                                    "%s: %s\r\n", header->name, header->value);
        }
        // format the response string
        char chunk[BUFF_SIZE];
        size_t bytes_sent = 0;
        size_t bytes_remaining = res_len;
        snprintf((char *)buff, sizeof(buff),
                 "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nDate: %s\r\n%s\r\n\r\n",
                 http_res->status_code, http_res->reason, http_res->content_type, res_len, date_str, headers);
        headers_len = strlen((char *)buff);
        if (send(client_server->conn_fd, (char *)buff, headers_len, 0) < 0)
            err_n_die("Error while sending headers");

        while (bytes_remaining > 0)
        {
            size_t bytes_to_send = MIN(BUFF_SIZE, bytes_remaining);
            memcpy(chunk, res + bytes_sent, bytes_to_send);
            if (send(client_server->conn_fd, chunk, bytes_to_send, 0) < 0)
            {
                err_n_die("Error while sending response chunk");
            }
            bytes_sent += bytes_to_send;
            bytes_remaining -= bytes_to_send;
        }

        // header_t *header = owl_hashmap_get(http_req->headers, &(header_t){.name = "Connection"});
    }
    else if (server->num_registered_file_paths > 0)
    {
        char filepath[MAX_PATH_LEN];
        int found = 0;
        for (int i = 0; i < server->num_registered_file_paths; i++)
        {
            snprintf(filepath, MAX_PATH_LEN, "%s%s", client_server->server->static_files_path[i], strcmp(http_req->uri, "/") == 0 ? "/index.html" : http_req->uri);

            struct stat st;
            if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode))
            {
                // If filepath is a directory, check filepath inside it
                snprintf(filepath, MAX_PATH_LEN, "%s/index.html", filepath);

                if (access(filepath, F_OK) == 0)
                {
                    owl_println("filepath: %s", filepath);
                    found = 1;
                    break;
                }
            }
            else
            {
                // If filepath is not a directory, check it directly
                if (access(filepath, F_OK) == 0)
                {
                    owl_println("filepath: %s", filepath);
                    found = 1;
                    break;
                }
            }
        }

        if (!found) goto send404;

        char *content_type = "text/plain";
        char *ext = strrchr(filepath, '.');
        if (ext)
        {
            if (strcmp(ext, ".html") == 0)
                content_type = "text/html";
            else if (strcmp(ext, ".css") == 0)
                content_type = "text/css";
            else if (strcmp(ext, ".js") == 0)
                content_type = "text/javascript";
            else if (strcmp(ext, ".png") == 0)
                content_type = "image/png";
            else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
                content_type = "image/jpeg";
            else if (strcmp(ext, ".gif") == 0)
                content_type = "image/gif";
            else if (strcmp(ext, ".svg") == 0)
                content_type = "image/svg+xml";
            else if (strcmp(ext, ".ico") == 0)
                content_type = "image/x-icon";
            else if (strcmp(ext, ".avif") == 0)
                content_type = "image/avif";
            else if (strcmp(ext, ".mp3") == 0)
                content_type = "audio/mpeg";
            else if (strcmp(ext, ".ogg") == 0)
                content_type = "audio/ogg";
            else if (strcmp(ext, ".wav") == 0)
                content_type = "audio/wav";
            else if (strcmp(ext, ".mp4") == 0)
                content_type = "video/mp4";
            else if (strcmp(ext, ".webm") == 0)
                content_type = "video/webm";
            else if (strcmp(ext, ".ogg") == 0)
                content_type = "video/ogg";
            else if (strcmp(ext, ".ttf") == 0)
                content_type = "font/ttf";
            else if (strcmp(ext, ".otf") == 0)
                content_type = "font/otf";
            else if (strcmp(ext, ".woff") == 0)
                content_type = "font/woff";
            else if (strcmp(ext, ".woff2") == 0)
                content_type = "font/woff2";
        }

        FILE *fp;
        // images should be sent as binary while text/* should be sent as
        // plain text
        if (strstr(content_type, "text/"))
            fp = fopen(filepath, "r");
        else
            fp = fopen(filepath, "rb");

        if (!fp) goto send404;

        // get the file size
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // format headers to send in the response
        char headers[MAX_HEADER_LEN] = "";
        size_t headers_len = 0;
        void *item;
        size_t iter = 0;
        while (owl_hashmap_iter(http_res->headers, &iter, &item))
        {
            const header_t *header = item;
            headers_len += snprintf(headers + headers_len, sizeof(headers) - headers_len,
                                    "%s: %s\r\n", header->name, header->value);
        }

        // Format the response string for other content types
        // Headers buffer contains a \r\n with the last header.
        // We just need to add one \r\n at the end to separate the body
        snprintf((char *)buff, sizeof(buff),
                 "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n%s\r\n", date_str, content_type, fsize, headers);

        if (send(client_server->conn_fd, (char *)buff, strlen((char *)buff), 0) < 0)
        {
            fclose(fp);
            err_n_die("Error while sending");
        }

        // send file in chunks
        char file_buffer[BUFF_SIZE];
        size_t bytes_read = 0;
        while (bytes_read < fsize)
        {
            size_t bytes_to_read = fsize - bytes_read;
            if (bytes_to_read > BUFF_SIZE)
                bytes_to_read = BUFF_SIZE;

            size_t result = fread(file_buffer, 1, bytes_to_read, fp);
            if (result == 0)
                break;

            size_t bytes_sent = 0;
            while (bytes_sent < result)
            {
                ssize_t sent = send(client_server->conn_fd, file_buffer + bytes_sent, result - bytes_sent, 0);
                if (sent == -1)
                {
                    fclose(fp);
                    err_n_die("Error while sending");
                }
                bytes_sent += sent;
            }

            bytes_read += result;
        }

        fclose(fp);
    }
    else
    send404:
    {
        // return 404 if the route doesn't exist
        const char *not_found = "404 Not Found";
        const char *content_type = "text/plain";
        const char *server = "kraken";
        const char *content = "The requested resource was not found";

        snprintf((char *)buff, sizeof(buff),
                 "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nDate: %s\r\nserver: %s\r\n\r\n%s",
                 not_found, content_type, strlen(content), date_str, server, content);

        if (send(client_server->conn_fd, (char *)buff, strlen((char *)buff), 0) < 0)
            err_n_die("Error while sending");
    }

        close(client_server->conn_fd);
    http_res_free(http_res);
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

int register_route(http_server_t *server, char *uri, char *(*handler)(http_req_t *req, http_res_t *res))
{
    if (owl_hashmap_oom(server->routes)) err_n_die("Routes Map Out of Memory");

    owl_hashmap_set(server->routes, &(http_route_t){.uri = uri, .handler = handler});

    return 0;
}

void register_static(http_server_t *server, char *path)
{
    server->static_files_path[server->num_registered_file_paths++] = path;
}

void http_server_free(http_server_t *server)
{
    owl_thread_pool_free(server->thread_pool);
    owl_hashmap_free(server->routes);
    http_status_map_free(server->http_status_map);
    server_free(server->server);
    free(server);
}

void handle_interrupt(int signal_num)
{
    owl_println("\n[🐙] Interrupt signal received. Shutting down");
    if (server) http_server_free(server);
    exit(signal_num);
}