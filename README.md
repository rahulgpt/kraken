<h2 align="left">
<img align="left" height="40" src="assets/kraken_logo.png">
Kraken
<h2>

A simple [http server](https://en.wikipedia.org/wiki/HTTP_server) written in c

## Quick Example

```c
#include <stdio.h>
#include <stdlib.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *index_handler(http_req_t *req, http_res_t *res)
{
    return "Hello World";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    // register a endpoint
    register_route(server, "/", index_handler);

    // register static files directory
    register_static(server, "public");

    http_server_listen(server);
    http_server_free(server);
}
```

## Docs

https://krakendocs.netlify.app/

## Disclaimer

Kraken is a project that was built as a final year project and is not intended for production use. It is still in development and has not been thoroughly tested for security vulnerabilities or performance issues. Please use at your own risk and only for educational or experimental purposes.
