# Http Response

Every route handler receives a `http_res_t *res` that it can use to configure the response, and returns the body as a string. This page documents the functions for setting the status, the headers and the body.

For examples, see [Status Codes, Headers and JSON](../examples/status-codes-and-headers.md) and [Server Side Rendering](../examples/server-side-rendering.md).

## Who frees the body?

A handler returns a `char *`, and kraken sends it and then frees anything it allocated:

- **String literals** like `return "<h1>Hi</h1>";` are never freed. They live for the whole program.
- **Strings returned by `res_sendf` and the `res_render_*` functions** belong to kraken. It frees them after the response is sent, so just return them.
- **Strings you `malloc` yourself** are not freed by kraken. Pass them through `res_sendf("%s", str)` and free your copy, or keep them alive for the whole program.

## res_status

```c
void res_status(http_res_t *res, http_status_t status_code)
```

Set the status code. Responses are `200 OK` unless you change it.

#### Parameters:

- _**res**_ - the response.
- _**status_code**_ - one of the `http_status_t` values below.

#### Example

```c
res_status(res, HTTP_STATUS_CREATED);
```

## http_status_t

```c
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
```

Kraken looks up the reason phrase ("Created", "Not Found", ...) for the status line. Codes outside this list send a `500 Internal Server Error` instead.

## res_content_type

```c
void res_content_type(http_res_t *res, char *content_type)
```

Set the `Content-Type` header. The default is `text/html`. The string isn't copied, so pass a string literal.

#### Example

```c
res_content_type(res, "application/json");
```

## res_header

```c
void res_header(http_res_t *res, const char *name, const char *value)
```

Add a header to the response. Both strings are copied, so they can come from a local buffer. Setting the same header twice replaces the first value.

Every response already includes `Content-Type`, `Content-Length`, `Date`, `Connection: close` and `Server: Kraken`.

#### Parameters:

- _**res**_ - the response.
- _**name**_ - the header name, e.g. `"Cache-Control"`.
- _**value**_ - the header value.

#### Example

```c
res_header(res, "Cache-Control", "no-store");

char count[16];
snprintf(count, sizeof(count), "%d", visits);
res_header(res, "X-Visits", count);
```

## res_sendf

```c
char *res_sendf(const char *fmt, ...)
```

Build a response body with `printf` style formatting. The returned string belongs to kraken and is freed after the response is sent.

#### Example

```c
char *hello_handler(http_req_t *req, http_res_t *res)
{
    char *name = req_query_param(req, "name");
    return res_sendf("<h1>Hello %s</h1>", name ? name : "stranger");
}
```

:::caution

Values from the request (query parameters, headers, the body) can contain HTML. Escape `<`, `>`, `&` and quotes before putting them in an HTML response, otherwise visitors can inject markup or scripts into your page.

:::

## res_render_template

```c
char *res_render_template(const char *template, placeholder_t *placeholders, size_t num_placeholders)
```

Replace placeholders in a template string. Every occurrence of every placeholder is replaced, in any order. The template is scanned once, so placeholder values are inserted as they are and never replaced again.

#### Parameters:

- _**template**_ - the template, e.g. `"<h1>{{title}}</h1>"`.
- _**placeholders**_ - an array of `placeholder_t` pairs.
- _**num_placeholders**_ - the length of the array, usually `NUM_PLACEHOLDERS(placeholders)`.

#### Returns:

The rendered string, owned by kraken.

#### Example

```c
placeholder_t placeholders[] = {
    {"{{title}}", "Kraken"},
    {"{{tagline}}", "an http server written in c"},
};

return res_render_template("<h1>{{title}}</h1><p>{{tagline}}</p>", placeholders,
                           NUM_PLACEHOLDERS(placeholders));
```

## res_render_template_file

```c
char *res_render_template_file(const char *filepath, placeholder_t *placeholders, size_t num_placeholders)
```

Same as `res_render_template`, but reads the template from a file.

#### Parameters:

- _**filepath**_ - path to the template, relative to the directory the server was started in.
- _**placeholders**_ - an array of `placeholder_t` pairs.
- _**num_placeholders**_ - the length of the array.

#### Returns:

The rendered string owned by kraken, or `NULL` if the file can't be opened. Returning that `NULL` from a handler sends a 404.

#### Example

```c
return res_render_template_file("./templates/home.html", placeholders, NUM_PLACEHOLDERS(placeholders));
```

## res_render_static_file

```c
char *res_render_static_file(const char *filepath)
```

Read a whole file and return its contents, without replacing any placeholders. Use it to serve a fixed file for a route. It works for text files: the length of the body is found with `strlen`, so binary files like images should be served from a [static directory](../examples/serving-static-files.md) instead.

#### Example

```c
char *index_handler(http_req_t *req, http_res_t *res)
{
    return res_render_static_file("./public/app.html");
}
```

## placeholder_t

```c
typedef struct
{
    char *placeholder;
    char *value;
} placeholder_t;
```

A placeholder and the value that replaces it. Placeholders are usually written as `{{name}}`, but any string works. A `NULL` value is treated as an empty string.

## NUM_PLACEHOLDERS

```c
#define NUM_PLACEHOLDERS(placeholders) sizeof(placeholders) / sizeof(placeholder_t)
```

The number of items in a placeholder array. It only works on arrays, not on pointers.
