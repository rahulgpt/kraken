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
    return res_render_static_file("./public/app.html");
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/", index_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

If you want to serve a specific file for a path, you can use the `res_render_static_file` procedure. It takes the path to the file as an argument, relative to the directory the server was started from. It returns `NULL` if the file can't be opened, which makes Kraken respond with a 404.

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

If you want to serve some static files from your machine such as HTML, CSS, or JavaScript files, you can register a static directory to the server. Kraken will automatically serve all of the files in this directory. The files will be accessible using the relative path and the file name with extension from the registered directory.

For example, in the above block of code, we are registering "public" as a static directory. Suppose our "public" folder looks like this:

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

The file called "test.html" can be accessed by going to "/test.html" in the browser, while the image called "burger.jpg" will be at "/images/burger.jpg".

A few more details:

- `/` and any folder serve the `index.html` inside them, so `/` serves `public/index.html` if it exists.
- The `Content-Type` is chosen from the file extension (`.css` is `text/css`, `.png` is `image/png`, and so on). Unknown extensions are sent as `application/octet-stream`.
- Routes are checked before static files. If you register a route for `/test.html`, the handler wins.
- You can register up to 10 directories. They are searched in the order they were registered and the first match is served.
- Paths containing `..` are rejected, so a request like `/../secret.txt` can't read files outside the directory.
