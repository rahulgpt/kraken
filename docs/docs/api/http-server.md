# Http Server

This module is the core of kraken where most of the things happen. Following are the list of functions available in this header file. If you want to see an example of how to use these functions see [click here.](../examples/creating-a-endpoint.md)

## http_server_init

```c
http_server_t *http_server_init(int port, int backlog)
```

This function initializes the server and returns a handle to the server

#### Parameters:

- _**port**_ - port on which you want to want the server to listen to.
- _**backlog**_ - the number of connection that the server should queue before rejecting if the server is busy.

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

Start listening for connections.

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

Register static directory in the server.

#### Parameters:

- _**server**_ - the handle to the server.
- _**path**_ - relative or absolute path to the directory.

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

#### Parameters:

- _**server**_ - the handle to the server.
- _**route**_ - the route you want to register.
- _**handler**_ - a handler function for the route.

#### Example

```c
// define a route handler function. It should return a char *
// You can read a template here. Kraken provides utility functions
// for that. Take a look at Http Response in the api section.
char *home_handler(http_req_t *req, http_res_t *res)
{
  return "<h1>This is home page.<h1>"
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
