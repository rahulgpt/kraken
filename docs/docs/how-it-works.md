# How It Works

This page follows one request through Kraken, from the TCP connection to the response. All of it lives in `src/`, in roughly 900 lines of C.

```text
accept() → thread pool → read request → parse → route lookup → handler → send response → close
```

## 1. Listening

`http_server_init` creates a TCP socket and gets it ready for connections (`src/server.c`):

```c
socket(AF_INET, SOCK_STREAM, 0);    // an IPv4 TCP socket
bind(fd, &address, sizeof(address)); // on INADDR_ANY:port
listen(fd, backlog);                 // start queueing connections
```

`backlog` is how many connections the kernel will queue while the server is busy before it starts refusing them. Kraken clamps it between 10 and 1000.

It also creates a pool of 20 worker threads and ignores `SIGPIPE`, so a client that disconnects in the middle of a response can't take down the whole process.

## 2. Accepting connections

`http_server_listen` runs the accept loop on the main thread:

```c
for (;;)
{
    int conn_fd = accept(socket_fd, &client_addr, &addrlen);
    setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, ...); // 10 second read timeout
    owl_thread_pool_enqueue_task(pool, client_handler, conn_fd);
}
```

`accept()` blocks until a client connects and returns a new file descriptor for that connection. The main thread doesn't read from it. It puts the connection on the thread pool's queue and goes straight back to `accept()`, so a slow client never stops other clients from connecting.

The read timeout matters: without it, a client that connects and never sends anything would hold a worker thread forever.

## 3. Reading the request

A worker thread picks the connection up and reads the request. HTTP doesn't arrive in neat pieces: TCP is a stream, and one request can come in one `recv()` call or in many. Kraken keeps reading until it has a complete request:

1. Read until the **head** is complete, which ends with an empty line (`\r\n\r\n`).
2. Look for a `Content-Length` header in the head. If there is one, keep reading until that many **body** bytes have arrived.

```text
POST /echo HTTP/1.1\r\n          ← request line
Host: localhost:8000\r\n         ← headers
Content-Length: 12\r\n
\r\n                              ← empty line, end of the head
hello kraken                      ← 12 bytes of body
```

Requests are capped at 64 KB. The raw bytes, along with a hex dump, are printed to stdout so you can watch requests arrive.

## 4. Parsing

`http_req_init` (`src/http_req.c`) turns those bytes into an `http_req_t`:

- The **request line** is split into the method, the target and the version. Unknown methods or a malformed line get a `400 Bad Request`.
- The target is split at `?` into the **path** and the **query string**. The path is URL decoded (`%20` becomes a space), and the query string is split into decoded key/value pairs.
- Every **header** line is split at the first `:`. Names are lowercased (HTTP header names are case insensitive) and values are trimmed. Headers and query parameters are stored in hash maps.
- Whatever follows the empty line is the **body**.

The parser works on its own copy of the request and keeps an untouched copy in `req->raw` for handlers that want to see exactly what was sent.

## 5. Routing

Routes live in a hash map keyed by path, using SipHash. Looking up `/hello` is a single hash map lookup, no matter how many routes are registered. Routing uses the decoded path without the query string, so `/hello?name=kraken` runs the `/hello` handler.

If there is no route for the path, Kraken tries the static directories in the order they were registered:

- `/css/style.css` maps to `<dir>/css/style.css`
- `/` and folders map to `index.html` inside them
- Any path containing `..` is refused, so requests can't escape the static directory (for example `/../../etc/passwd`)

If nothing matches, the response is a `404 Not Found` page.

## 6. Running the handler

The handler gets the parsed request and an `http_res_t` with sensible defaults (`200 OK`, `text/html`). It can change the status, the content type and the headers, then return the body:

```c
char *handler(http_req_t *req, http_res_t *res)
{
    res_status(res, HTTP_STATUS_CREATED);
    res_content_type(res, "application/json");
    return "{\"ok\": true}";
}
```

Returning `NULL` sends a 404.

Handlers can return string literals, or strings built by Kraken's helpers (`res_sendf`, `res_render_template`, ...). Kraken remembers every string those helpers allocate on the current thread and frees them after the response is sent, so handlers never have to worry about who frees the body.

## 7. Sending the response

Kraken writes the status line and headers, followed by the body:

```text
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 20
Date: Mon, 06 Oct 2026 18:30:00 GMT
Connection: close
Server: Kraken

<h1>Hello World</h1>
```

`send()` is allowed to write only part of a buffer, so Kraken loops until everything is sent. Static files are read and sent in 4 KB chunks, so large files are never loaded into memory all at once. For `HEAD` requests only the headers are sent.

## 8. Cleaning up

Kraken uses `Connection: close`: after every response the worker closes the socket, frees the request, the response and any rendered strings, and goes back to waiting for the next connection in the queue.

## Limits

| Limit | Value |
| --- | --- |
| Worker threads | 20 |
| Maximum request size (head and body) | 64 KB |
| Read timeout | 10 seconds |
| Static directories | 10 |
| Maximum static file path | 512 characters |
| Response headers | 1 KB |

These are `#define`s at the top of `src/http_server.c` and `src/http_server.h`.

## What Kraken doesn't do

Kraken leaves out a lot of what a production server needs, which keeps the code small enough to read in an afternoon:

- No TLS. Put it behind a proxy that handles HTTPS (see [Deploying](deploying.md)).
- No keep-alive, every connection serves one request.
- No chunked request bodies, only `Content-Length`.
- No route parameters like `/users/:id`, and every method goes to the same handler (check `req->method` yourself).
