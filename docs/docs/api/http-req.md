# Http Request

Every route handler receives the parsed request as `http_req_t *req`. The request and every string in it belong to kraken and are freed after the response is sent, so copy anything you need to keep.

For examples, see [Reading Request Data](../examples/reading-request-data.md).

## http_req_t

```c
typedef struct HTTPReq
{
    int method;
    char *uri;
    char *path;
    char *query;
    float http_version;
    owl_hashmap_t *headers;
    owl_hashmap_t *query_params;
    char *body;
    size_t body_len;
    char *raw;
    size_t raw_len;
    char remote_addr[64];
    int remote_port;
} http_req_t;
```

| Field | Description | Example |
| --- | --- | --- |
| `method` | The request method, one of the `HTTPMethod` values below | `GET` |
| `uri` | The request target exactly as sent | `"/hello%20world?name=kraken"` |
| `path` | `uri` without the query string, URL decoded. This is what routes match against | `"/hello world"` |
| `query` | The raw query string after `?`, or `NULL` if there is none | `"name=kraken"` |
| `http_version` | The HTTP version from the request line | `1.1` |
| `headers` | Request headers, use [`req_header`](#req_header) to read them | |
| `query_params` | Decoded query parameters, use [`req_query_param`](#req_query_param) to read them | |
| `body` | The request body. Always followed by a `\0`, but may contain `\0` bytes itself | `"hello"` |
| `body_len` | Length of the body in bytes, `0` if there is no body | `5` |
| `raw` | The whole request exactly as it was read from the socket | |
| `raw_len` | Length of `raw` in bytes | |
| `remote_addr` | IP address of the client connected to the socket | `"127.0.0.1"` |
| `remote_port` | Port of the client connected to the socket | `52144` |

:::note

`remote_addr` is the address of whatever opened the TCP connection. Behind a proxy or a load balancer this is the proxy, and the visitor's address is usually in a header such as `X-Forwarded-For`.

:::

## HTTPMethod

```c
enum HTTPMethod
{
    GET,
    POST,
    PUT,
    HEAD,
    PATCH,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
};
```

Requests with any other method are answered with `400 Bad Request` before they reach a handler. For `HEAD` requests kraken runs the handler like a `GET`, but only sends the headers.

#### Example

```c
if (req->method == POST)
{
    // create something
}
```

## req_header

```c
char *req_header(http_req_t *req, const char *name)
```

Get the value of a request header. Header names are case insensitive.

#### Parameters:

- _**req**_ - the request.
- _**name**_ - the header name, e.g. `"User-Agent"`.

#### Returns:

The header value with surrounding whitespace removed, or `NULL` if the request doesn't have that header.

#### Example

```c
char *agent = req_header(req, "User-Agent");
if (agent) printf("browser: %s\n", agent);
```

## req_query_param

```c
char *req_query_param(http_req_t *req, const char *name)
```

Get a parameter from the query string. Values are URL decoded, so both `+` and `%20` become a space.

#### Parameters:

- _**req**_ - the request.
- _**name**_ - the parameter name. Unlike headers, parameter names are case sensitive.

#### Returns:

The decoded value, an empty string for a parameter without a value (`?debug`), or `NULL` if the parameter is missing.

#### Example

```c
// GET /search?q=octopus+facts
char *q = req_query_param(req, "q"); // "octopus facts"
```

## req_method

```c
const char *req_method(http_req_t *req)
```

Get the request method as a string, e.g. `"GET"`.

#### Example

```c
printf("%s %s\n", req_method(req), req->path); // GET /hello
```

## Iterating over all headers

`headers` and `query_params` are [owl](https://github.com/rahulgpt/owl) hash maps of `header_t` pairs. Header names are stored lowercased.

```c
typedef struct
{
    char *name;
    char *value;
} header_t;
```

```c
void *item;
size_t iter = 0;
while (owl_hashmap_iter(req->headers, &iter, &item))
{
    header_t *header = item;
    printf("%s: %s\n", header->name, header->value);
}
```

The order of iteration is not the order the headers were sent in. Use `req->raw` if you need that.
