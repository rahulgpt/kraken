# Status Codes, Headers and JSON

Handlers configure the response through `http_res_t *res` before returning the body. This example builds a tiny JSON API.

```c
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

static int visits = 0;

// GET /api/visits
char *visits_handler(http_req_t *req, http_res_t *res)
{
    res_content_type(res, "application/json");
    res_header(res, "Cache-Control", "no-store");
    res_header(res, "Access-Control-Allow-Origin", "*");

    return res_sendf("{\"visits\": %d}\n", ++visits);
}

// GET /api/teapot
char *teapot_handler(http_req_t *req, http_res_t *res)
{
    res_status(res, HTTP_STATUS_NOT_IMPLEMENTED);
    res_content_type(res, "application/json");

    return "{\"error\": \"no tea here\"}\n";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/api/visits", visits_handler);
    register_route(server, "/api/teapot", teapot_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

```bash
$ curl -i localhost:8000/api/visits
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 14
Date: Mon, 06 Oct 2026 18:30:00 GMT
Connection: close
Server: Kraken
Cache-Control: no-store
Access-Control-Allow-Origin: *

{"visits": 1}
```

## Status codes

`res_status` sets the status code. Responses are `200 OK` unless you change it. The `http_status_t` enum has names for the supported codes:

| Code | Name |
| --- | --- |
| 100 | `HTTP_STATUS_CONTINUE` |
| 200 | `HTTP_STATUS_OK` |
| 201 | `HTTP_STATUS_CREATED` |
| 202 | `HTTP_STATUS_ACCEPTED` |
| 204 | `HTTP_STATUS_NO_CONTENT` |
| 400 | `HTTP_STATUS_BAD_REQUEST` |
| 401 | `HTTP_STATUS_UNAUTHORIZED` |
| 403 | `HTTP_STATUS_FORBIDDEN` |
| 404 | `HTTP_STATUS_NOT_FOUND` |
| 405 | `HTTP_STATUS_METHOD_NOT_ALLOWED` |
| 500 | `HTTP_STATUS_INTERNAL_SERVER_ERROR` |
| 501 | `HTTP_STATUS_NOT_IMPLEMENTED` |
| 502 | `HTTP_STATUS_BAD_GATEWAY` |
| 503 | `HTTP_STATUS_SERVICE_UNAVAILABLE` |
| 504 | `HTTP_STATUS_GATEWAY_TIMEOUT` |

Setting a code that isn't in this list sends a `500 Internal Server Error` and logs a message.

## Content type

`res_content_type` sets the `Content-Type` header. The default is `text/html`. Pass a string literal, Kraken keeps the pointer rather than copying it.

## Headers

`res_header` adds any other header. Both the name and the value are copied, so they can come from a local buffer. Setting the same header twice replaces the old value. Every response also gets `Content-Length`, `Date`, `Connection: close` and `Server: Kraken`.

## Thread safety

Handlers run on 20 worker threads at the same time. The `visits` counter above is fine for a demo, but two requests can update it at once. For anything that matters, use `<stdatomic.h>` (`atomic_int visits`) or a mutex.
