# Http Server

This module is the core of kraken where most of the things happen. It owns the listening socket, the thread pool, the route table and the static directories. Following are the list of functions available in this header file. For an example of how to use them, see [Creating an Endpoint](../examples/creating-a-endpoint.md).

## http_server_init

```c
http_server_t *http_server_init(int port, int backlog)
```

This function initializes the server and returns a handle to the server

#### Parameters:

- _**port**_ - port on which you want the server to listen. `0` uses the default, `8000`.
- _**backlog**_ - the number of connections the kernel should queue before rejecting new ones. Values below 10 use 10 and values above 1000 use 1000.

The server listens on all interfaces (`0.0.0.0`). It exits with an error message if the socket can't be created or the port is already in use.

#### Example:

```c
#define PORT 8000
#define BACKLOG 10

http_server_t *server = http_server_init(PORT, BACKLOG);
```

## http_server_listen

```c
void http_server_listen(http_server_t *server)
```

Start accepting connections. This runs the accept loop and doesn't return, so register every route and static directory before calling it. Press `Ctrl+C` to stop the server.

#### Parameters:

- _**server**_ - the handle to the server.

#### Example:

```c
#define PORT 8000
#define BACKLOG 10

http_server_t *server = http_server_init(PORT, BACKLOG);

http_server_listen(server);
```

## register_static

```c
void register_static(http_server_t *server, char *path)
```

Register a directory to serve static files from. Requests that don't match a route are looked up in the registered directories, in the order they were registered. See [Serving Static Files](../examples/serving-static-files.md).

#### Parameters:

- _**server**_ - the handle to the server.
- _**path**_ - relative or absolute path to the directory, without a trailing slash. Relative paths are resolved from the directory the server was started in. Up to 10 directories can be registered.

#### Example

```c
// register a dir name public as a static dir
register_static(server, "public");

// you can register multiple static dir
register_static(server, "../static");
```

## register_route

```c
int register_route(http_server_t *server, char *route, char *(*handler)(http_req_t *req, http_res_t *res))
```

Register a dynamic endpoint. The route handler function receives a `http_req_t` and a `http_res_t`. You can read more about them at [Http Request](http-req.md) and [Http Response](http-res.md).

Routes match the path exactly, without the query string: a handler registered for `/hello` runs for `/hello` and `/hello?name=kraken`, but not for `/hello/` or `/hello/world`. Every method (`GET`, `POST`, ...) goes to the same handler. Registering the same route twice replaces the old handler.

The handler returns the response body. It can return a string literal, a string returned by one of the `res_*` helpers (which kraken frees after sending), or `NULL` to send a 404.

#### Parameters:

- _**server**_ - the handle to the server.
- _**route**_ - the path you want to register, e.g. `"/about"`. It must stay valid while the server runs, so pass a string literal.
- _**handler**_ - a handler function for the route.

#### Example

```c
// define a route handler function. It should return a char *
// You can read a template here. Kraken provides utility functions
// for that. Take a look at Http Response in the api section.
char *home_handler(http_req_t *req, http_res_t *res)
{
    return "<h1>This is home page.</h1>";
}

register_route(server, "/home", home_handler);
```

## http_server_free

```c
void http_server_free(http_server_t *server)
```

Deallocate any resource allocated by the server. Should be called when you are done using the server to avoid memory leaks.

#### Parameters:

- _**server**_ - the handle to the server.

#### Example

```c
http_server_free(server);
```
