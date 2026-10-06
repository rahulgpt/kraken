#include "http_res.h"
#include "http_status.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Strings returned by the render functions are tracked per thread so the
// server can free them once the response is sent. Handlers can return either
// a rendered string or a string literal without worrying about ownership.
static __thread char **rendered = NULL;
static __thread size_t num_rendered = 0;
static __thread size_t rendered_cap = 0;

static char *track_rendered(char *str)
{
    if (num_rendered == rendered_cap)
    {
        rendered_cap = rendered_cap ? rendered_cap * 2 : 8;
        rendered = realloc(rendered, rendered_cap * sizeof(char *));
    }
    rendered[num_rendered++] = str;
    return str;
}

void res_free_rendered(void)
{
    for (size_t i = 0; i < num_rendered; i++) free(rendered[i]);
    num_rendered = 0;
}

typedef struct
{
    char *name;
    char *value;
} header_t;

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

http_res_t *http_res_init()
{
    http_res_t *res = malloc(sizeof(http_res_t));
    res->content_type = "text/html";
    res->status_code = 200;
    res->reason = "OK";
    res->headers = owl_hashmap_new(sizeof(header_t), sizeof(header_t), 0, 0,
                                   header_hash, header_compare, NULL, NULL);

    // append default headers
    res_header(res, "Server", "Kraken");

    return res;
}

void http_res_free(http_res_t *res)
{
    // header names and values are copies made by res_header
    void *item;
    size_t iter = 0;
    while (owl_hashmap_iter(res->headers, &iter, &item))
    {
        header_t *header = item;
        free(header->name);
        free(header->value);
    }

    owl_hashmap_free(res->headers);
    free(res);
}

void res_header(http_res_t *res, const char *name, const char *value)
{
    if (!res || !name || !value) return;

    char *name_copy = malloc(strlen(name) + 1);
    char *value_copy = malloc(strlen(value) + 1);
    strcpy(name_copy, name);
    strcpy(value_copy, value);

    // setting a header twice replaces the old value
    header_t *old = owl_hashmap_set(res->headers, &(header_t){.name = name_copy, .value = value_copy});
    if (old)
    {
        free(old->name);
        free(old->value);
    }
}

void res_status(http_res_t *res, http_status_t status_code)
{
    if (res) res->status_code = status_code;
}

void res_content_type(http_res_t *res, char *content_type)
{
    if (res) res->content_type = content_type;
}

char *res_render_template(const char *template, placeholder_t *placeholders, size_t num_placeholders)
{
    size_t len = 0;
    size_t cap = strlen(template) + 1;
    char *result = malloc(cap);

    // walk the template once. At every position check if one of the
    // placeholders starts there, and copy either its value or the character.
    // Values are never scanned again, so they can safely contain "{{...}}"
    const char *pos = template;
    while (*pos)
    {
        const char *piece = pos;
        size_t piece_len = 1;
        size_t skip = 1;

        for (size_t i = 0; i < num_placeholders; i++)
        {
            size_t placeholder_len = strlen(placeholders[i].placeholder);
            if (placeholder_len && strncmp(pos, placeholders[i].placeholder, placeholder_len) == 0)
            {
                piece = placeholders[i].value ? placeholders[i].value : "";
                piece_len = strlen(piece);
                skip = placeholder_len;
                break;
            }
        }

        if (len + piece_len + 1 > cap)
        {
            while (len + piece_len + 1 > cap) cap *= 2;
            result = realloc(result, cap);
        }
        memcpy(result + len, piece, piece_len);
        len += piece_len;
        pos += skip;
    }

    result[len] = '\0';
    return track_rendered(result);
}

char *res_render_template_file(const char *filepath, placeholder_t *placeholders, size_t num_placeholders)
{
    char *template = NULL;
    char *result = NULL;
    FILE *file = fopen(filepath, "r");

    if (file == NULL)
    {
        fprintf(stderr, "Failed to open template file '%s'\n", filepath);
        return NULL;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the template
    template = (char *)malloc(filesize + 1);

    // Read the file into the buffer
    fread(template, 1, filesize, file);
    fclose(file);
    template[filesize] = '\0';

    // Call the existing render_template function with the buffer and the placeholders
    result = res_render_template(template, placeholders, num_placeholders);

    // Free the template buffer
    free(template);

    return result;
}

char *res_render_static_file(const char *filepath)
{
    return res_render_template_file(filepath, NULL, 0);
}
char *res_sendf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0) return NULL;

    char *result = malloc(len + 1);
    va_start(args, fmt);
    vsnprintf(result, len + 1, fmt, args);
    va_end(args);

    return track_rendered(result);
}
