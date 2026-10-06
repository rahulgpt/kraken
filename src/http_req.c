#include "http_req.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEADER_NAME_LEN 256

static const char *method_names[] = {"GET", "POST", "PUT", "HEAD", "PATCH", "DELETE", "CONNECT", "OPTIONS", "TRACE"};
static const int num_methods = sizeof(method_names) / sizeof(method_names[0]);

static int select_method(char *method)
{
    for (int i = 0; i < num_methods; i++)
    {
        if (strcmp(method, method_names[i]) == 0) return i;
    }

    // unsupported method, the caller decides how to respond
    return -1;
}

static int header_compare(const void *a, const void *b, void *udata)
{
    const header_t *ha = a;
    const header_t *hb = b;
    return strcmp(ha->name, hb->name);
}

static uint64_t header_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const header_t *header = item;
    return owl_hashmap_sip(header->name, strlen(header->name), seed0, seed1);
}

bool header_iter(const void *item, void *udata)
{
    const header_t *header = item;
    printf("name: %s, value: %s\n", header->name, header->value);
    return true;
}

static owl_hashmap_t *pair_map_new()
{
    return owl_hashmap_new(sizeof(header_t), 0, 0, 0, header_hash, header_compare, NULL, NULL);
}

static void str_tolower(char *str)
{
    for (; *str; str++) *str = tolower((unsigned char)*str);
}

static char *trim(char *str)
{
    while (*str == ' ' || *str == '\t') str++;

    char *end = str + strlen(str);
    while (end > str && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) end--;
    *end = '\0';

    return str;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// decode %XX escapes in place. In query strings '+' also means a space
static void url_decode(char *str, int plus_is_space)
{
    char *out = str;
    for (char *in = str; *in; in++)
    {
        if (*in == '%' && hex_value(in[1]) >= 0 && hex_value(in[2]) >= 0)
        {
            *out++ = (char)(hex_value(in[1]) * 16 + hex_value(in[2]));
            in += 2;
        }
        else if (*in == '+' && plus_is_space)
            *out++ = ' ';
        else
            *out++ = *in;
    }
    *out = '\0';
}

// header lines look like "Name: value". Lines are separated by \r\n (or a bare \n)
static owl_hashmap_t *parse_http_headers(char *header_fields)
{
    owl_hashmap_t *header_map = pair_map_new();
    if (!header_fields) return header_map;

    char *line_pos = NULL;
    for (char *line = strtok_r(header_fields, "\n", &line_pos); line; line = strtok_r(NULL, "\n", &line_pos))
    {
        char *colon = strchr(line, ':');
        if (!colon) continue;

        *colon = '\0';
        char *name = trim(line);
        str_tolower(name);

        owl_hashmap_set(header_map, &(header_t){.name = name, .value = trim(colon + 1)});
    }

    return header_map;
}

// "a=1&b=two" -> {a: "1", b: "two"}
static owl_hashmap_t *parse_query_params(char *query)
{
    owl_hashmap_t *params = pair_map_new();
    if (!query) return params;

    char *pair_pos = NULL;
    for (char *pair = strtok_r(query, "&", &pair_pos); pair; pair = strtok_r(NULL, "&", &pair_pos))
    {
        char *value = strchr(pair, '=');
        if (value) *value++ = '\0';
        else value = "";

        url_decode(pair, 1);
        url_decode(value, 1);
        owl_hashmap_set(params, &(header_t){.name = pair, .value = value});
    }

    return params;
}

http_req_t *http_req_init(const char *raw, size_t raw_len)
{
    if (!raw || raw_len < 2) return NULL;

    http_req_t *req = calloc(1, sizeof(http_req_t));

    // keep an untouched copy of the request around for handlers that want it
    req->raw = malloc(raw_len + 1);
    memcpy(req->raw, raw, raw_len);
    req->raw[raw_len] = '\0';
    req->raw_len = raw_len;

    // parsing cuts the string up in place, so it works on a second copy
    char *buf = malloc(raw_len + 1);
    memcpy(buf, raw, raw_len);
    buf[raw_len] = '\0';
    req->head_buf = buf;

    // the head (request line + headers) ends at the first empty line
    char *head_end = strstr(buf, "\r\n\r\n");
    size_t separator_len = 4;
    if (!head_end)
    {
        head_end = strstr(buf, "\n\n");
        separator_len = 2;
    }

    if (head_end)
    {
        *head_end = '\0';
        req->body = head_end + separator_len;
        req->body_len = raw_len - (req->body - buf);
    }
    else
    {
        req->body = buf + raw_len;
        req->body_len = 0;
    }

    // request line, e.g. "GET /hello?name=kraken HTTP/1.1"
    char *line_pos = NULL;
    char *req_line = strtok_r(buf, "\n", &line_pos);
    if (!req_line) goto malformed;
    req_line = trim(req_line);

    char *token_pos = NULL;
    char *method = strtok_r(req_line, " ", &token_pos);
    char *uri = strtok_r(NULL, " ", &token_pos);
    char *http_version = strtok_r(NULL, " ", &token_pos);

    if (!method || !uri || !http_version || strncmp(http_version, "HTTP/", 5) != 0) goto malformed;

    req->method = select_method(method);
    if (req->method < 0) goto malformed;

    req->uri = malloc(strlen(uri) + 1);
    strcpy(req->uri, uri);
    req->http_version = (float)atof(http_version + 5);

    // split "/hello?name=kraken" into the path and the query string
    char *question_mark = strchr(uri, '?');
    if (question_mark)
    {
        *question_mark = '\0';
        req->query = question_mark + 1;
        req->query_buf = malloc(strlen(req->query) + 1);
        strcpy(req->query_buf, req->query);
    }
    url_decode(uri, 0);
    req->path = uri;

    req->headers = parse_http_headers(line_pos);
    req->query_params = parse_query_params(req->query_buf);

    return req;

malformed:
    http_req_free(req);
    return NULL;
}

char *req_header(http_req_t *req, const char *name)
{
    if (!req || !req->headers || !name) return NULL;

    // headers are stored lowercased, so lowercase the name we look up too
    char lowered[MAX_HEADER_NAME_LEN];
    snprintf(lowered, sizeof(lowered), "%s", name);
    str_tolower(lowered);

    header_t *header = owl_hashmap_get(req->headers, &(header_t){.name = lowered});
    return header ? header->value : NULL;
}

char *req_query_param(http_req_t *req, const char *name)
{
    if (!req || !req->query_params || !name) return NULL;

    header_t *param = owl_hashmap_get(req->query_params, &(header_t){.name = (char *)name});
    return param ? param->value : NULL;
}

const char *req_method(http_req_t *req)
{
    if (!req || req->method < 0 || req->method >= num_methods) return NULL;
    return method_names[req->method];
}

void http_req_free(http_req_t *http_req)
{
    if (!http_req) return;

    if (http_req->headers) owl_hashmap_free(http_req->headers);
    if (http_req->query_params) owl_hashmap_free(http_req->query_params);
    free(http_req->uri);
    free(http_req->raw);
    free(http_req->head_buf);
    free(http_req->query_buf);
    free(http_req);
}
