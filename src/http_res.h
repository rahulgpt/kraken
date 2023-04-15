#ifndef http_res_h
#define http_res_h

#include "../external/include/owl/collections/hashmap.h"
#include <stdlib.h>

typedef struct
{
    int status_code;
    char *reason;
    owl_hashmap_t *headers;
    char *response;
    char *content_type;
} http_res_t;

typedef enum
{
    HTTP_STATUS_CONTINUE = 100,
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,
    HTTP_STATUS_GATEWAY_TIMEOUT = 504
} http_status_t;

typedef struct
{
    char *placeholder;
    char *value;
} placeholder_t;

http_res_t *http_res_init();
void http_res_free(http_res_t *res);

void res_status(http_res_t *res, http_status_t status_code);
void res_content_type(http_res_t *res, char *content_type);

// utility functions to render templates dynamically
char *res_render_template(const char *template, placeholder_t *placeholders, size_t num_placeholders);
char *res_render_template_file(const char *filepath, placeholder_t *placeholders, size_t num_placeholders);

#define NUM_PLACEHOLDERS(placeholders) sizeof(placeholders) / sizeof(placeholder_t)

#endif /* http_res_h */