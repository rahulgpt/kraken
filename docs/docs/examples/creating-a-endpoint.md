# Creating a Endpoint

```c
#include <stdio.h>
#include <stdlib.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *index_handler(http_req_t *req, http_res_t *res)
{
    return res_send("Hello World");
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

Here we define the `PORT` on which we want the server to listen and the `BACKLOG` size. `BACKLOG` specifies the number of connections that the server will queue if it's busy before rejecting them. It is important to note that although Kraken is a multi-threaded server that can handle multiple blocking connections with its multi-threaded architecture, we may still want to specify the `BACKLOG` in case all of the threads are busy. If you don't want to queue incoming connections, you can pass `NULL` as the `BACKLOG` parameter.

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
    return res_send("Hello World");
}
```

This is the handler function. The handler function passed to `register_route` should have the exact function signature. It should return a `char *` at the end. It takes `http_req` and `http_res` as arguments. Every route handler function will receive request for checking various information about the incoming `request` and `response` for configuring the response. You can look more about the API in the API Reference section.
