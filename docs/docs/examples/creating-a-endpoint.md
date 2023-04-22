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

Include a single header file called kraken.h to get access to all of the kraken procedures.

```c
#define PORT 8000
#define BACKLOG 10
```

We are defining the `port` on which we want out server to listen and the size of the `backlog`.
Backlog specifies the number of connection that the server will queue if it's busy before rejecting.

Although Kraken is a multi-threaded server and can handle multiple blocking connection with it's multi-threaded archietecture, we may still wanna specify the backlog in case all of the threads are busy. If you don't want to queue incoming connection, pass `NULL` as the backlog parameter.

```c
http_server_t *server = http_server_init(PORT, BACKLOG);
```

We can initialize the server by using `http_server_init` procedure. It takes two arguments, port and backlog. We can pass them here as we already defined them above.

```c
register_route(server, "/", index_handler);
```

Next we are reistering a route in the server with this `register_route` procedure. It takes in three argument, the server on which we want to register a route, the route/path we want to register, and the route handler function for that path.

The route handler function should have the signature `char *handler(http_req_t *req, http_res_t *res)`. The route handler will be called when you open the `/` path. More about the route handler later.

```c
http_server_listen(server);
```

Next we are calling the `http_server_listen` procedure and passing the server we created as the argument. This function will make the server start listening for incoming connection on the specified port.

:::note

All of the register procedures should be called before calling this function otherwise the routes won't register.

:::

```c
http_server_free(server);
```

In the end when we are done with our server, we should call `http_server_free` to free any resources allocated by the server.

```c
char *index_handler(http_req_t *req, http_res_t *res)
{
    return res_send("Hello World");
}
```

Now let's talk about the handler function. The handler function passed to `register_route` should have the exact function signature. It should return a `char *` at the end. It will take the http_req and http_res as arguments. Every route handler function will receive `request` for checking various information about the incoming req and `response` for configuring the response. You can look more about the api in the API Reference section.
