# Reading Request Data

Every handler receives the parsed request as `http_req_t *req`. This example reads the method, query parameters, headers and body.

```c
#include <stdio.h>
#include <string.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

// GET /search?q=octopus&page=2
char *search_handler(http_req_t *req, http_res_t *res)
{
    char *q = req_query_param(req, "q");
    char *page = req_query_param(req, "page");

    res_content_type(res, "text/plain");
    return res_sendf("searching for \"%s\" (page %s)\n", q ? q : "", page ? page : "1");
}

// GET /whoami
char *whoami_handler(http_req_t *req, http_res_t *res)
{
    char *agent = req_header(req, "User-Agent");

    res_content_type(res, "text/plain");
    return res_sendf("%s %s from %s:%d\nUser-Agent: %s\n",
                     req_method(req), req->path, req->remote_addr, req->remote_port,
                     agent ? agent : "unknown");
}

// POST /notes with a body
char *notes_handler(http_req_t *req, http_res_t *res)
{
    if (req->method != POST)
    {
        res_status(res, HTTP_STATUS_METHOD_NOT_ALLOWED);
        res_header(res, "Allow", "POST");
        return "use POST\n";
    }

    res_status(res, HTTP_STATUS_CREATED);
    res_content_type(res, "text/plain");
    return res_sendf("saved a %zu byte note: %.*s\n", req->body_len, (int)req->body_len, req->body);
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/search", search_handler);
    register_route(server, "/whoami", whoami_handler);
    register_route(server, "/notes", notes_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

```bash
$ curl 'localhost:8000/search?q=octopus&page=2'
searching for "octopus" (page 2)

$ curl localhost:8000/whoami
GET /whoami from 127.0.0.1:52144
User-Agent: curl/8.7.1

$ curl -X POST -d 'buy milk' localhost:8000/notes
saved a 8 byte note: buy milk
```

## Query parameters

The query string is not part of the route: `/search?q=octopus` runs the handler registered for `/search`. `req_query_param` returns the decoded value of a parameter, so `?q=hello+world` and `?q=hello%20world` both give `"hello world"`. It returns `NULL` when the parameter is missing, so always check before using it.

The raw query string is also available as `req->query` (`"q=octopus&page=2"`), or `NULL` if the URL has no `?`.

## Headers

`req_header` looks headers up by name, ignoring case, so `req_header(req, "user-agent")` and `req_header(req, "User-Agent")` return the same value. It returns `NULL` if the header wasn't sent.

## Method

`req->method` is one of `GET`, `POST`, `PUT`, `HEAD`, `PATCH`, `DELETE`, `CONNECT`, `OPTIONS` or `TRACE`. Every method goes to the same handler, so check it when a route should only accept some of them. `req_method(req)` gives the method as a string.

## Body

`req->body` points to the bytes after the headers and `req->body_len` is their length. Kraken reads as many bytes as the `Content-Length` header says. The body is always followed by a `\0`, so text bodies can be used as a C string, but binary bodies can contain `\0` bytes too, so prefer `body_len` when the body could be binary.

## Everything else

`req->raw` holds the whole request exactly as it was read from the socket, which is handy for debugging. The [live demo](https://kraken.rahulgpt.com) uses it to show visitors their own request. See [Http Request](../api/http-req.md) for every field.
