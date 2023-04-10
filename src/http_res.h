#ifndef http_res_h
#define http_res_h

#include <stdlib.h>

typedef struct
{

} http_res;

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

char *http_res_render(int count, ...);
void http_res_status(http_status_t status);

// utility functions to render templates dynamically
char *render_template(const char *template, placeholder_t *placeholders, size_t num_placeholders);
char *render_template_file(const char *filepath, placeholder_t *placeholders, size_t num_placeholders);

#endif /* http_res_h */