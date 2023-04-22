# Serving Static Files

## Serving static files by registering a route

```c
#include <stdio.h>
#include <stdlib.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *index_handler(http_req_t *req, http_res_t *res)
{
    return res_render_static_file("./react/react.html");
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/", index_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

If you have a template that you want to serve for a path, you can use the `res_render_static_file` procedure. It takes the path to you template file as argument.

## Serving static files from a directory

```c
#include <stdio.h>
#include <stdlib.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_static(server, "public");

    http_server_listen(server);
    http_server_free(server);

}
```

If you want to serve some static files from your machine such as html, css or javascript files, you can regiter a static dir to the server. Kraken will automatically serve all of the files in this directory. The files will be accessible using the relative path and the file name with extension from the registered directory.

For example, in the above block of code we are registering public as a static directly. Suppose our public folder looks like this.

```bash title="public/"
.
├── band.html
├── catering.html
├── coming-soon.html
├── css
│   └── style.css
├── images
│   ├── burger.jpg
│   ├── cta.png
│   └── leaf.avif
├── js
│   └── index.js
├── parallex.html
└── test.html
```

The file called test.html can be accessed by going to `/test.html` in the browser, while the image called burger.jpg will be at `/images/burger.jpg`.
