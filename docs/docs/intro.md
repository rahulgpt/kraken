---
slug: /
---

# Introduction

Welcome to the Kraken Docs 🎉

Kraken is a small, multi-threaded HTTP/1.1 server framework written from scratch in C. It gives you an Express like API: register a function for a path, return a string, and Kraken takes care of the sockets, the HTTP parsing and sending the response.

```c
char *hello_handler(http_req_t *req, http_res_t *res)
{
    char *name = req_query_param(req, "name"); // "/hello?name=kraken"
    return res_sendf("<h1>Hello %s</h1>", name ? name : "stranger");
}

register_route(server, "/hello", hello_handler);
```

:::tip See it running

**[kraken.rahulgpt.com](https://kraken.rahulgpt.com)** is served by Kraken. The page shows the raw bytes of your request exactly as the server read them off the socket, what the parser made of them, and live stats about the server process.

:::

## Features

- **Plain sockets**: `socket`, `bind`, `listen` and `accept` on a TCP port, no external networking library.
- **Thread pool**: every connection is handled by one of 20 worker threads from [owl](https://github.com/rahulgpt/owl).
- **HTTP/1.1 parsing**: request line, headers, `Content-Length` bodies, URL decoding and query strings.
- **Routing**: exact path routes stored in a hash map for O(1) lookup.
- **Static files**: serve one or more directories with MIME type detection and `index.html` for folders.
- **Templates**: replace `{{placeholders}}` in a string or a file to render HTML on the server.

## Using the docs

The docs are divided into a few sections:

- [_Getting Started_](getting-started.md) builds Kraken and runs your first server.
- [_Examples_](examples/creating-a-endpoint.md) are small, complete programs that show one feature at a time.
- [_How It Works_](how-it-works.md) follows a request from `accept()` to the response, for anyone curious about the internals.
- [_Deploying_](deploying.md) explains how to host a Kraken server, including for free on a serverless platform.
- The [_API Reference_](api/http-server.md) (reachable via the menu bar) documents every function and type.

## What is Kraken for

Kraken started as my final year project, written by hand to understand how web servers actually work. Possible use cases:

- Learning how HTTP works below the frameworks you normally use.
- An introduction to web development when C is your first language.
- Serving static content or a small dynamic site from your machine.
- Experimenting with web development in a language that isn't usually used for it.

:::caution

Kraken is an educational project and is not intended for production use. It has not been audited for security or tuned for performance. Please use it for learning and experiments.

:::
