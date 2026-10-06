<h2 align="left">
<img align="left" height="40" src="assets/kraken_logo.png">
Kraken
<h2>

A multi-threaded [http server](https://en.wikipedia.org/wiki/HTTP_server) written from scratch in c, with an Express like API.

**Live demo: [kraken.rahulgpt.com](https://kraken.rahulgpt.com)**, a page served by Kraken that shows your request exactly as the server read it off the socket.

## Quick Example

```c
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *hello_handler(http_req_t *req, http_res_t *res)
{
    char *name = req_query_param(req, "name"); // /hello?name=kraken
    return res_sendf("<h1>Hello %s</h1>", name ? name : "World");
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    // register an endpoint
    register_route(server, "/hello", hello_handler);

    // register static files directory
    register_static(server, "public");

    http_server_listen(server);
    http_server_free(server);
}
```

## Features

- TCP server on plain POSIX sockets, with a pool of 20 worker threads
- HTTP/1.1 parsing: headers, `Content-Length` bodies, URL decoding and query strings
- Exact path routing with O(1) hash map lookup
- Static file serving with MIME types, `index.html` for folders and path traversal protection
- Server side rendering with `{{placeholder}}` templates

## Building

```sh
git clone --recursive https://github.com/rahulgpt/kraken.git
cd kraken
make
./main   # the demo app, open http://localhost:8000
```

## Deploying (AWS Lambda)

Kraken runs unmodified on Lambda using the [Lambda Web Adapter](https://github.com/awslabs/aws-lambda-web-adapter), which forwards each invocation to the server as a normal HTTP request. It scales to zero, so a low traffic demo costs nothing.

```sh
deploy/lambda/build.sh   # cross-compile for Amazon Linux (needs Docker)
deploy/lambda/deploy.sh  # create or update the function and print its public URL
```

`deploy/cloudflare` is a small Cloudflare Worker that serves it on a custom domain (`npx wrangler deploy` from that folder, needs Node 22+). See [Deploying](https://krakendocs.netlify.app/deploying) for details.

## Docs

https://krakendocs.netlify.app/

## Disclaimer

Kraken is a project that was built as a final year project and is not intended for production use. It is still in development and has not been thoroughly tested for security vulnerabilities or performance issues. Please use at your own risk and only for educational or experimental purposes.
