#include "http_server.h"
#include "../external/include/owl/collections/hashmap.h"
#include "../external/include/owl/systems/thread_pool.h"
#include "../external/include/owl/utils/log.h"
#include "http_req.h"
#include "http_status.h"
#include "server.h"
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define PROTOCOL 0
#define PORT 8000
#define BACKLOG 10
#define THREADS 20
#define BUFF_SIZE 4096

#define MAX_LINE 4096
#define MAX_REQ_LEN 65536
#define MAX_HEADER_LEN 1024
#define MAX_PATH_LEN 512
#define RECV_TIMEOUT_SECS 10
#define MAX_BACKLOG 1000

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
    if (!port) port = PORT;

    http_server_t *http_server = malloc(sizeof(http_server_t));
    http_server->server = server_init(AF_INET, SOCK_STREAM, PROTOCOL, INADDR_ANY,
                                      port, backlog);
    http_server->routes = owl_hashmap_new(sizeof(http_route_t), sizeof(http_route_t), 0, 0,
                                          route_hash, route_compare, NULL, NULL);

    http_server->thread_pool = owl_thread_pool_init(THREADS);
    http_server->http_status_map = http_status_map_init();
    http_server->num_registered_file_paths = 0;
    http_server->port = port;

    // bind the global server var to the latest instance
    server = http_server;

    signal(SIGINT, handle_interrupt);
    // a client hanging up mid response should not kill the whole server
    signal(SIGPIPE, SIG_IGN);

    return http_server;
}

// This will be passed to the thread function
typedef struct
{
    int conn_fd;
    struct sockaddr_in addr;
    http_server_t *server;
} client_server_t;

void *client_handler(void *arg);

static void handle_clients(http_server_t *http_server)
{
    owl_thread_pool_t *tp = http_server->thread_pool;

    owl_println("[🐙] Server started listening on port %d", http_server->port);

    for (;;)
    {
        owl_println("[🐙] Waiting for connections ...");
        fflush(stdout);

        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int conn_fd = accept(http_server->server->socket_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (conn_fd < 0)
        {
            perror("accept");
            continue;
        }

        // don't let a client that never sends anything hold a worker forever
        struct timeval timeout = {.tv_sec = RECV_TIMEOUT_SECS, .tv_usec = 0};
        setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        client_server_t *client_server = malloc(sizeof(client_server_t));
        client_server->server = http_server;
        client_server->conn_fd = conn_fd;
        client_server->addr = client_addr;

        owl_worker_task_t handle_client = owl_worker_task_init(client_handler, client_server);
        owl_thread_pool_enqueue_task(tp, &handle_client);
    }
}

// send the whole buffer, send() is allowed to write only part of it
static int send_all(int fd, const char *buf, size_t len)
{
    while (len > 0)
    {
        ssize_t sent = send(fd, buf, len, 0);
        if (sent < 0)
        {
            if (errno == EINTR) continue;
            return -1;
        }
        buf += sent;
        len -= sent;
    }
    return 0;
}

// find the empty line that ends the request head, returns the head length
static size_t find_head_end(const char *buf)
{
    char *end = strstr(buf, "\r\n\r\n");
    if (end) return end - buf + 4;

    end = strstr(buf, "\n\n");
    if (end) return end - buf + 2;

    return 0;
}

// value of the Content-Length header in the request head, 0 if missing
static size_t content_length(const char *buf, size_t head_len)
{
    const char *line = buf;
    while (line && line < buf + head_len)
    {
        if (strncasecmp(line, "Content-Length:", 15) == 0) return strtoul(line + 15, NULL, 10);

        line = strchr(line, '\n');
        if (line) line++;
    }
    return 0;
}

/*
 * Read one request from the socket. A request is a head (request line and
 * headers) that ends with an empty line, followed by Content-Length bytes of
 * body. TCP can split this across any number of recv() calls.
 */
static size_t read_request(int fd, char *buf, size_t cap)
{
    size_t len = 0;
    size_t head_len = 0;
    size_t body_len = 0;

    while (len < cap)
    {
        ssize_t n = recv(fd, buf + len, cap - len, 0);
        if (n == 0)
        {
            owl_println("Connection closed by client");
            break;
        }
        if (n < 0)
        {
            if (errno == EINTR) continue;
            break; // timeout or reset
        }

        len += n;
        buf[len] = '\0';

        if (!head_len)
        {
            head_len = find_head_end(buf);
            if (head_len) body_len = content_length(buf, head_len);
        }

        if (head_len && len >= head_len + body_len) break;
    }

    return len;
}

// format the headers set on the response, each one ends with \r\n
static void format_headers(http_res_t *http_res, char *headers, size_t size)
{
    size_t headers_len = 0;
    void *item;
    size_t iter = 0;

    headers[0] = '\0';
    while (owl_hashmap_iter(http_res->headers, &iter, &item))
    {
        const header_t *header = item;
        int written = snprintf(headers + headers_len, size - headers_len, "%s: %s\r\n", header->name, header->value);
        if (written < 0 || (size_t)written >= size - headers_len) break;
        headers_len += written;
    }
}

static int send_error(int conn_fd, int status_code, const char *reason, const char *date_str, int send_body)
{
    char content[512];
    char buff[MAX_LINE + 1];

    snprintf(content, sizeof(content),
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "    <title>%d %s</title>\n"
             "</head>\n"
             "<body>\n"
             "    <h1>%d %s</h1>\n"
             "    <p>%s</p>\n"
             "</body>\n"
             "</html>",
             status_code, reason, status_code, reason,
             status_code == 404 ? "The requested resource was not found on this server." : "The server could not handle this request.");

    snprintf(buff, sizeof(buff),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Date: %s\r\n"
             "Server: Kraken\r\n"
             "Connection: close\r\n"
             "Cache-Control: no-cache\r\n"
             "\r\n%s",
             status_code, reason, strlen(content), date_str, send_body ? content : "");

    return send_all(conn_fd, buff, strlen(buff));
}

static const char *mime_type(const char *filepath)
{
    static const char *types[][2] = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".pdf", "application/pdf"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".txt", "text/plain"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".avif", "image/avif"},
        {".webp", "image/webp"},
        {".mp3", "audio/mpeg"},
        {".ogg", "audio/ogg"},
        {".wav", "audio/wav"},
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
    };

    const char *ext = strrchr(filepath, '.');
    if (!ext) return "text/plain";

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++)
    {
        if (strcmp(ext, types[i][0]) == 0) return types[i][1];
    }
    return "application/octet-stream";
}

// look for the requested path in every registered static directory
static int find_static_file(http_server_t *http_server, const char *path, char *filepath, size_t size)
{
    for (int i = 0; i < http_server->num_registered_file_paths; i++)
    {
        int written = snprintf(filepath, size, "%s%s", http_server->static_files_path[i],
                               strcmp(path, "/") == 0 ? "/index.html" : path);
        if (written < 0 || (size_t)written >= size) continue;

        struct stat st;
        if (stat(filepath, &st) != 0) continue;

        if (S_ISDIR(st.st_mode))
        {
            // If filepath is a directory, check for an index.html inside it
            char dir[MAX_PATH_LEN];
            snprintf(dir, sizeof(dir), "%s", filepath);
            written = snprintf(filepath, size, "%s/index.html", dir);
            if (written < 0 || (size_t)written >= size) continue;
            if (access(filepath, F_OK) != 0) continue;
        }

        owl_println("filepath: %s", filepath);
        return 1;
    }

    return 0;
}

static int send_static_file(int conn_fd, http_res_t *http_res, const char *filepath, const char *date_str, int send_body)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return -1;

    // get the file size
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char headers[MAX_HEADER_LEN];
    format_headers(http_res, headers, sizeof(headers));

    // Headers buffer contains a \r\n with the last header.
    // We just need to add one \r\n at the end to separate the body
    char buff[MAX_LINE + 1];
    snprintf(buff, sizeof(buff),
             "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n%s\r\n",
             date_str, mime_type(filepath), fsize, headers);

    int result = send_all(conn_fd, buff, strlen(buff));

    // send file in chunks
    char file_buffer[BUFF_SIZE];
    size_t bytes_read;
    while (result == 0 && send_body && (bytes_read = fread(file_buffer, 1, BUFF_SIZE, fp)) > 0)
    {
        result = send_all(conn_fd, file_buffer, bytes_read);
    }

    fclose(fp);
    return result;
}

// thread function
void *client_handler(void *arg)
{
    client_server_t *client_server = arg;
    http_server_t *http_server = client_server->server;
    int conn_fd = client_server->conn_fd;
    char *req_string = malloc(MAX_REQ_LEN + 1);
    http_req_t *http_req = NULL;
    http_res_t *http_res = NULL;

    // Date header indicates the data and time the response was generated
    char date_str[100];
    time_t now = time(NULL);
    struct tm tm;
    gmtime_r(&now, &tm);
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", &tm);

    size_t req_len = read_request(conn_fd, req_string, MAX_REQ_LEN);
    if (req_len == 0) goto cleanup;

    // print the request
    char *hex = bin2hex((unsigned char *)req_string, req_len);
    fprintf(stdout, "\n%s\n\n%s\n", hex, req_string);
    free(hex);

    http_req = http_req_init(req_string, req_len);
    if (!http_req)
    {
        // empty, malformed or unsupported request
        send_error(conn_fd, 400, "Bad Request", date_str, 1);
        goto cleanup;
    }

    inet_ntop(AF_INET, &client_server->addr.sin_addr, http_req->remote_addr, sizeof(http_req->remote_addr));
    http_req->remote_port = ntohs(client_server->addr.sin_port);

    // a HEAD request gets the same headers as GET, but no body
    int send_body = http_req->method != HEAD;

    http_res = http_res_init();

    http_route_t *route = owl_hashmap_get(http_server->routes, &(http_route_t){.uri = http_req->path});

    if (route)
    {
        // After calling the handler, "http_res" will be populated
        char *res = route->handler(http_req, http_res);
        if (!res) goto send404;

        // get the reason string from the status map
        status_code_with_reason_t *scr = owl_hashmap_get(http_server->http_status_map,
                                                         &(status_code_with_reason_t){.code = http_res->status_code});
        if (!scr)
        {
            owl_println("%d http status is not supported by the server.", http_res->status_code);
            send_error(conn_fd, 500, "Internal Server Error", date_str, send_body);
            goto cleanup;
        }
        http_res->reason = scr->reason;

        // format headers to send in the response
        char headers[MAX_HEADER_LEN];
        format_headers(http_res, headers, sizeof(headers));

        // format the response string
        size_t res_len = strlen(res);
        char buff[MAX_LINE + 1];
        snprintf(buff, sizeof(buff),
                 "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nDate: %s\r\nConnection: close\r\n%s\r\n",
                 http_res->status_code, http_res->reason, http_res->content_type, res_len, date_str, headers);

        if (send_all(conn_fd, buff, strlen(buff)) < 0 || (send_body && send_all(conn_fd, res, res_len) < 0))
            perror("Error while sending response");
    }
    else
    {
        char filepath[MAX_PATH_LEN];

        // block path traversal like /../../etc/passwd (the path is already url decoded)
        if (strstr(http_req->path, "..")) goto send404;
        if (!find_static_file(http_server, http_req->path, filepath, sizeof(filepath))) goto send404;

        if (send_static_file(conn_fd, http_res, filepath, date_str, send_body) < 0)
            perror("Error while sending file");
    }

    goto cleanup;

send404:
    // return 404 if the route doesn't exist
    send_error(conn_fd, 404, "Not Found", date_str, send_body);

cleanup:
    close(conn_fd);
    res_free_rendered();
    if (http_res) http_res_free(http_res);
    http_req_free(http_req);
    free(req_string);
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
    if (server->num_registered_file_paths >= MAX_STATIC_FILES_PATH)
    {
        owl_println("[🐙] Can't register \"%s\", at most %d static directories are supported", path, MAX_STATIC_FILES_PATH);
        return;
    }

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