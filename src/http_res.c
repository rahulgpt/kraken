#include "http_res.h"
#include "http_status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    header_t headers[] = {
        {"server", "Kraken"}};

    const size_t num_headers = sizeof(headers) / sizeof(header_t);

    for (size_t i = 0; i < num_headers; i++)
    {
        owl_hashmap_set(res->headers, &headers[i]);
    }

    return res;
}

void http_res_free(http_res_t *res)
{
    owl_hashmap_free(res->headers);
    free(res);
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
    char *result = NULL;
    const char *start = template;
    const char *end = NULL;
    int offset = 0;

    for (size_t i = 0; i < num_placeholders; i++)
    {
        const char *placeholder = placeholders[i].placeholder;
        const char *value = placeholders[i].value;
        int len = strlen(placeholder);
        int value_len = strlen(value);

        while ((end = strstr(start, placeholder)) != NULL)
        {
            int new_len = end - start + value_len;
            result = (char *)realloc(result, offset + new_len + 1);
            memcpy(result + offset, start, end - start);
            memcpy(result + offset + (end - start), value, value_len);
            offset += new_len;
            start = end + len;
        }
    }

    if (start != template + strlen(template))
    {
        int new_len = template + strlen(template) - start;
        result = (char *)realloc(result, offset + new_len + 1);
        memcpy(result + offset, start, new_len);
        offset += new_len;
    }

    result = (char *)realloc(result, offset + 1);
    result[offset] = '\0';
    return result;
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