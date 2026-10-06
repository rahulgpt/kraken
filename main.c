/*
 * Kraken demo app (https://kraken.rahulgpt.com)
 *
 * Every page here is served by kraken itself. The index page shows the raw
 * request exactly as it was read off the socket, what the parser made of it,
 * and some live stats about the server process.
 */
#include "kraken.h"
#include "owl/utils/log.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define PORT 8000
#define BACKLOG 10
#define HEX_DUMP_LIMIT 512

static time_t boot_time;
static atomic_long requests_served = 0;
static atomic_int workers_seen = 0;

/* ---------- small string builder used to generate html and json ---------- */

typedef struct
{
    char *data;
    size_t len;
    size_t cap;
} sb_t;

static void sb_append_n(sb_t *sb, const char *str, size_t n)
{
    if (sb->len + n + 1 > sb->cap)
    {
        sb->cap = (sb->len + n + 1) * 2;
        sb->data = realloc(sb->data, sb->cap);
    }
    memcpy(sb->data + sb->len, str, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

static void sb_append(sb_t *sb, const char *str)
{
    sb_append_n(sb, str, strlen(str));
}

static void sb_appendf(sb_t *sb, const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    sb_append(sb, buf);
}

// everything that came from the client must be escaped before it goes in html
static void sb_append_html(sb_t *sb, const char *str, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        switch (str[i])
        {
        case '<': sb_append(sb, "&lt;"); break;
        case '>': sb_append(sb, "&gt;"); break;
        case '&': sb_append(sb, "&amp;"); break;
        case '"': sb_append(sb, "&quot;"); break;
        case '\'': sb_append(sb, "&#39;"); break;
        case '\r': sb_append(sb, "<span class=\"ctl\">\\r</span>"); break;
        case '\n': sb_append(sb, "<span class=\"ctl\">\\n</span>\n"); break;
        case '\0': sb_append(sb, "<span class=\"ctl\">\\0</span>"); break;
        default: sb_append_n(sb, &str[i], 1);
        }
    }
}

static void sb_append_json(sb_t *sb, const char *str)
{
    sb_append(sb, "\"");
    for (; str && *str; str++)
    {
        if (*str == '"' || *str == '\\') sb_appendf(sb, "\\%c", *str);
        else if ((unsigned char)*str < 0x20) sb_appendf(sb, "\\u%04x", *str);
        else sb_append_n(sb, str, 1);
    }
    sb_append(sb, "\"");
}

/* ---------- helpers ---------- */

// the lambda web adapter adds headers with AWS account details, keep them off the page
static int is_hidden_header(const char *name)
{
    return strncasecmp(name, "x-amzn-", 7) == 0;
}

// copy of the raw request with hidden header values replaced
static char *redacted_raw(http_req_t *req, size_t *out_len)
{
    sb_t sb = {0};
    const char *line = req->raw;
    const char *end = req->raw + req->raw_len;
    const char *body = req->body_len ? req->raw + (req->raw_len - req->body_len) : end;

    while (line < body)
    {
        const char *next = memchr(line, '\n', body - line);
        next = next ? next + 1 : body;

        if (is_hidden_header(line))
        {
            const char *colon = memchr(line, ':', next - line);
            sb_append_n(&sb, line, colon ? colon - line + 1 : 0);
            sb_append(&sb, " [hidden]\r\n");
        }
        else
            sb_append_n(&sb, line, next - line);

        line = next;
    }
    sb_append_n(&sb, body, end - body);

    *out_len = sb.len;
    return sb.data;
}

// classic "xxd" style dump: offset, 16 hex bytes, printable characters
static void sb_append_hex_dump(sb_t *sb, const char *data, size_t len)
{
    size_t limit = len < HEX_DUMP_LIMIT ? len : HEX_DUMP_LIMIT;

    for (size_t offset = 0; offset < limit; offset += 16)
    {
        sb_appendf(sb, "<span class=\"off\">%08zx</span>  ", offset);
        for (size_t i = 0; i < 16; i++)
        {
            if (offset + i < limit) sb_appendf(sb, "%02x ", (unsigned char)data[offset + i]);
            else sb_append(sb, "   ");
            if (i == 7) sb_append(sb, " ");
        }
        sb_append(sb, " <span class=\"asc\">");
        for (size_t i = 0; i < 16 && offset + i < limit; i++)
        {
            char c = data[offset + i];
            if (c >= 0x20 && c < 0x7f) sb_append_html(sb, &c, 1);
            else sb_append(sb, ".");
        }
        sb_append(sb, "</span>\n");
    }

    if (len > limit) sb_appendf(sb, "<span class=\"off\">... %zu more bytes</span>\n", len - limit);
}

static void format_duration(char *buf, size_t size, long secs)
{
    if (secs < 60) snprintf(buf, size, "%lds", secs);
    else if (secs < 3600) snprintf(buf, size, "%ldm %lds", secs / 60, secs % 60);
    else snprintf(buf, size, "%ldh %ldm", secs / 3600, (secs % 3600) / 60);
}

// thread pool workers get a small number the first time they handle a request
static int worker_number(void)
{
    static __thread int number = 0;
    if (!number) number = atomic_fetch_add(&workers_seen, 1) + 1;
    return number;
}

static double elapsed_us(struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start->tv_sec) * 1e6 + (now.tv_nsec - start->tv_nsec) / 1e3;
}

/* ---------- route handlers ---------- */

char *index_handler(http_req_t *req, http_res_t *res)
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    long request_number = atomic_fetch_add(&requests_served, 1) + 1;

    res_content_type(res, "text/html; charset=utf-8");
    res_header(res, "Cache-Control", "no-store");

    // 1. the bytes as they arrived
    size_t raw_len;
    char *raw = redacted_raw(req, &raw_len);
    sb_t raw_html = {0}, hex_html = {0};
    sb_append_html(&raw_html, raw, raw_len);
    sb_append_hex_dump(&hex_html, raw, raw_len);
    free(raw);

    // 2. what the parser made of them
    sb_t headers_html = {0};
    void *item;
    size_t iter = 0;
    while (owl_hashmap_iter(req->headers, &iter, &item))
    {
        header_t *header = item;
        const char *value = is_hidden_header(header->name) ? "[hidden]" : header->value;
        sb_append(&headers_html, "<tr><td>");
        sb_append_html(&headers_html, header->name, strlen(header->name));
        sb_append(&headers_html, "</td><td>");
        sb_append_html(&headers_html, value, strlen(value));
        sb_append(&headers_html, "</td></tr>\n");
    }

    sb_t path_html = {0}, query_html = {0};
    sb_append_html(&path_html, req->path, strlen(req->path));
    if (req->query) sb_append_html(&query_html, req->query, strlen(req->query));
    else sb_append(&query_html, "<span class=\"dim\">none</span>");

    // 3. the server process
    char uptime[32], boot[64], version[16], peer[96], body_len[32], num[32], header_count[16], worker[16], micros[32];
    format_duration(uptime, sizeof(uptime), (long)(time(NULL) - boot_time));
    struct tm boot_tm;
    gmtime_r(&boot_time, &boot_tm);
    strftime(boot, sizeof(boot), "%Y-%m-%d %H:%M:%S UTC", &boot_tm);
    snprintf(version, sizeof(version), "HTTP/%.1f", req->http_version);
    snprintf(peer, sizeof(peer), "%s:%d", req->remote_addr, req->remote_port);
    snprintf(body_len, sizeof(body_len), "%zu bytes", req->body_len);
    snprintf(num, sizeof(num), "%ld", request_number);
    snprintf(header_count, sizeof(header_count), "%zu", owl_hashmap_count(req->headers));
    snprintf(worker, sizeof(worker), "#%d", worker_number());

    char pid[16], raw_size[32];
    snprintf(pid, sizeof(pid), "%d", (int)getpid());
    snprintf(raw_size, sizeof(raw_size), "%zu", req->raw_len);

    const char *start_kind = request_number == 1 ? "cold start" : "warm";
    const char *start_note = request_number == 1
                                 ? "This process was just started to serve you. Before your request, nothing was running."
                                 : "This process was already running and handled your request straight away.";

    snprintf(micros, sizeof(micros), "%.0f", elapsed_us(&start));

    placeholder_t placeholders[] = {
        {"{{raw}}", raw_html.data},
        {"{{hex}}", hex_html.data},
        {"{{raw_size}}", raw_size},
        {"{{method}}", (char *)req_method(req)},
        {"{{path}}", path_html.data},
        {"{{query}}", query_html.data},
        {"{{version}}", version},
        {"{{peer}}", peer},
        {"{{body_len}}", body_len},
        {"{{header_count}}", header_count},
        {"{{headers}}", headers_html.data ? headers_html.data : ""},
        {"{{request_number}}", num},
        {"{{uptime}}", uptime},
        {"{{boot}}", boot},
        {"{{pid}}", pid},
        {"{{worker}}", worker},
        {"{{micros}}", micros},
        {"{{start_kind}}", (char *)start_kind},
        {"{{start_note}}", (char *)start_note},
    };

    char *page = res_render_template_file("./templates/index.html", placeholders, NUM_PLACEHOLDERS(placeholders));

    free(raw_html.data);
    free(hex_html.data);
    free(headers_html.data);
    free(path_html.data);
    free(query_html.data);

    return page;
}

// GET /hello?name=you : query parameters and inline templates
char *hello_handler(http_req_t *req, http_res_t *res)
{
    atomic_fetch_add(&requests_served, 1);
    res_content_type(res, "text/html; charset=utf-8");

    char *name = req_query_param(req, "name");
    sb_t escaped = {0};
    if (name && *name) sb_append_html(&escaped, name, strlen(name));
    else sb_append(&escaped, "stranger");

    placeholder_t placeholders[] = {{"{{name}}", escaped.data}};
    char *page = res_render_template(
        "<!DOCTYPE html><meta name=\"viewport\" content=\"width=device-width\">"
        "<link rel=\"stylesheet\" href=\"/style.css\"><main class=\"mini\">"
        "<h1>Hello, {{name}} 👋</h1>"
        "<p>This came from <code>req_query_param(req, \"name\")</code>, dropped into a template "
        "with <code>res_render_template</code>. Try changing <code>?name=</code> in the address bar.</p>"
        "<p><a href=\"/\">&larr; back</a></p></main>",
        placeholders, NUM_PLACEHOLDERS(placeholders));

    free(escaped.data);
    return page;
}

// GET /api/request : the parsed request as json
char *api_request_handler(http_req_t *req, http_res_t *res)
{
    atomic_fetch_add(&requests_served, 1);
    res_content_type(res, "application/json");
    res_header(res, "Access-Control-Allow-Origin", "*");

    sb_t json = {0};
    sb_append(&json, "{\n  \"method\": ");
    sb_append_json(&json, req_method(req));
    sb_append(&json, ",\n  \"path\": ");
    sb_append_json(&json, req->path);
    sb_append(&json, ",\n  \"query\": {");

    void *item;
    size_t iter = 0;
    int first = 1;
    while (owl_hashmap_iter(req->query_params, &iter, &item))
    {
        header_t *param = item;
        sb_append(&json, first ? "\n    " : ",\n    ");
        sb_append_json(&json, param->name);
        sb_append(&json, ": ");
        sb_append_json(&json, param->value);
        first = 0;
    }
    sb_append(&json, first ? "},\n  \"headers\": {" : "\n  },\n  \"headers\": {");

    iter = 0;
    first = 1;
    while (owl_hashmap_iter(req->headers, &iter, &item))
    {
        header_t *header = item;
        sb_append(&json, first ? "\n    " : ",\n    ");
        sb_append_json(&json, header->name);
        sb_append(&json, ": ");
        sb_append_json(&json, is_hidden_header(header->name) ? "[hidden]" : header->value);
        first = 0;
    }
    sb_appendf(&json, "\n  },\n  \"http_version\": %.1f,\n  \"body_bytes\": %zu,\n  \"served_by\": \"kraken worker #%d\"\n}\n",
               req->http_version, req->body_len, worker_number());

    char *body = res_sendf("%s", json.data);
    free(json.data);
    return body;
}

// POST /echo : sends the request body back
char *echo_handler(http_req_t *req, http_res_t *res)
{
    atomic_fetch_add(&requests_served, 1);
    res_content_type(res, "text/plain; charset=utf-8");

    if (req->method != POST)
    {
        res_status(res, HTTP_STATUS_METHOD_NOT_ALLOWED);
        res_header(res, "Allow", "POST");
        return "Send me a POST with a body, e.g.\n\n  curl -X POST -d 'hello kraken' https://kraken.rahulgpt.com/echo\n";
    }

    return res_sendf("kraken received %zu bytes:\n\n%.*s\n", req->body_len, (int)req->body_len, req->body);
}

// GET /home : file based template (template.html)
char *home_handler(http_req_t *req, http_res_t *res)
{
    atomic_fetch_add(&requests_served, 1);
    res_status(res, HTTP_STATUS_OK);
    res_content_type(res, "text/html");

    char now[64];
    time_t t = time(NULL);
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(now, sizeof(now), "%H:%M:%S UTC", &tm);

    placeholder_t placeholders[] = {
        {"{{title}}", "Server side rendering"},
        {"{{content}}", "This page is template.html with its {{placeholders}} replaced on the server by res_render_template_file."},
        {"{{something1}}", now},
    };

    return res_render_template_file("./template.html", placeholders, NUM_PLACEHOLDERS(placeholders));
}

// GET /healthz : used by the lambda web adapter to check the server is up.
// Not counted, so the first visitor after a cold start sees request #1
char *health_handler(http_req_t *req, http_res_t *res)
{
    res_content_type(res, "text/plain");
    return "ok";
}

char *about_handler(http_req_t *req, http_res_t *res)
{
    atomic_fetch_add(&requests_served, 1);
    return "<h1>About Page</h1><a href=\"/home\">Home</a>";
}

int main()
{
    boot_time = time(NULL);

    // hosting platforms (Lambda, Cloud Run, ...) pass the port to listen on
    char *port_env = getenv("PORT");
    int port = port_env ? atoi(port_env) : PORT;

    http_server_t *server = http_server_init(port, BACKLOG);

    register_route(server, "/", index_handler);
    register_route(server, "/hello", hello_handler);
    register_route(server, "/api/request", api_request_handler);
    register_route(server, "/echo", echo_handler);
    register_route(server, "/home", home_handler);
    register_route(server, "/about", about_handler);
    register_route(server, "/healthz", health_handler);

    register_static(server, "static");
    register_static(server, "public");

    http_server_listen(server);
    http_server_free(server);
}
