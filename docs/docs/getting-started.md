# Getting Started

This page guides you through building Kraken and running your first server.

## Requirements

- macOS or Linux (Kraken uses POSIX sockets and pthreads)
- `clang` (or `gcc`, see below) and `make`
- `git`

On Ubuntu or Debian:

```bash
sudo apt install clang make git
```

On macOS, the Xcode command line tools are enough:

```bash
xcode-select --install
```

## Get the code

Kraken depends on [owl](https://github.com/rahulgpt/owl), my small C library for the thread pool and hash map. It is included as a git submodule, so clone with `--recursive`:

```bash
git clone --recursive https://github.com/rahulgpt/kraken.git
cd kraken
```

If you already cloned without it, fetch the submodule with:

```bash
git submodule update --init
```

## Build and run the demo

```bash
make
./main
```

`make` builds owl, the demo server (`./main`) and the static library `lib/libkraken.a`. Open [http://localhost:8000](http://localhost:8000) and you should see the same page as the [live demo](https://kraken.rahulgpt.com): your request as Kraken received it.

:::tip

To build with gcc instead of clang, run `make CC=gcc`. To build a shared library instead of a static one, run `make SHARED=1`.

:::

The port defaults to `8000` and can be changed with the `PORT` environment variable:

```bash
PORT=3000 ./main
```

## Project layout

```bash
.
├── kraken.h          # include this to use kraken
├── src/              # the server, request parser and response helpers
├── external/owl/     # thread pool and hash map (git submodule)
├── main.c            # the demo app
├── templates/        # html templates used by the demo
├── static/           # static files served by the demo (css, images)
├── template.html     # template used by the server side rendering example
└── deploy/           # scripts to deploy the demo to AWS Lambda and Cloudflare
```

## Your first server

The quickest way to start is to edit `main.c` and run `make` again. Replace its contents with:

```c
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *hello_handler(http_req_t *req, http_res_t *res)
{
    return "<h1>Hello World</h1>";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/", hello_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

```bash
make && ./main
```

```bash
$ curl -i http://localhost:8000/
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 20
Date: Mon, 06 Oct 2026 18:30:00 GMT
Connection: close
Server: Kraken

<h1>Hello World</h1>
```

Every route handler receives the parsed request and a response it can configure, and returns the response body as a string. [Creating an Endpoint](examples/creating-a-endpoint.md) explains each line of this program.

## Using Kraken in your own project

You can also keep your code outside the repository and link against the library that `make` builds:

```bash
clang -I path/to/kraken -o app app.c \
    -L path/to/kraken/lib -lkraken \
    -L path/to/kraken/external/lib -lowl
```

On older Linux systems you may also need `-lpthread`.

## Next steps

- Read the [examples](examples/creating-a-endpoint.md) to see routes, static files, templates and request data in action.
- Learn what happens to a request inside the server in [How It Works](how-it-works.md).
- Put your server online with [Deploying](deploying.md).
