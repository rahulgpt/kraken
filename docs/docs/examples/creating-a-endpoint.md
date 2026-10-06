# Creating an Endpoint

```c
#include <stdio.h>
#include <stdlib.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *index_handler(http_req_t *req, http_res_t *res)
{
    return "<h1>Hello World</h1>";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/", index_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

## Explanation

```c
#include "kraken.h"
```

This line includes the header file `kraken.h`, which gives access to all the procedures of Kraken.

```c
#define PORT 8000
#define BACKLOG 10
```

Here we define the `PORT` on which we want the server to listen and the `BACKLOG` size. `BACKLOG` specifies the number of connections that the kernel will queue before rejecting new ones. Kraken hands every connection to a pool of worker threads, so the queue only fills up if all the threads are busy. Passing `0` uses the default of 10, and values above 1000 are capped at 1000.

```c
http_server_t *server = http_server_init(PORT, BACKLOG);
```

This line initializes the server using the `http_server_init` function, which takes two arguments: `PORT` and `BACKLOG`. We can pass them as we already defined them above.

```c
register_route(server, "/", index_handler);
```

This line registers a route in the server using the `register_route` procedure, which takes three arguments: the `server` on which we want to register a route, the route/path we want to register, and the route handler function for that path. The `route handler function` should have the signature `char *handler(http_req_t *req, http_res_t *res)`. It will be called when we open the `/` path.

```c
http_server_listen(server);
```

This line starts the server listening for incoming connections on the specified port by calling the `http_server_listen` procedure with the `server` we created as the argument.

:::note

All of the register procedures should be called before calling this function otherwise the routes won't register.

:::

```c
http_server_free(server);
```

This line frees any resources allocated by the server when we are done with it, by calling the `http_server_free` procedure.

```c
char *index_handler(http_req_t *req, http_res_t *res)
{
    return "<h1>Hello World</h1>";
}
```

This is the handler function. The handler function passed to `register_route` should have this exact signature. It receives the parsed request (`req`), which you can read things like the path, headers and query parameters from, and the response (`res`), which you can use to set the status code, content type and headers. It returns the body of the response as a `char *`.

The returned string can be a string literal like here, or one built by Kraken's helpers such as `res_sendf` or `res_render_template_file`, which Kraken frees for you after the response is sent. Returning `NULL` sends a `404 Not Found`. See [Http Request](../api/http-req.md) and [Http Response](../api/http-res.md) for everything a handler can do.

## Building a response dynamically

Most handlers build their response from the request. `res_sendf` works like `printf` and returns a string that Kraken owns:

```c
char *time_handler(http_req_t *req, http_res_t *res)
{
    res_content_type(res, "text/plain");
    return res_sendf("The server time is %ld\n", (long)time(NULL));
}
```
