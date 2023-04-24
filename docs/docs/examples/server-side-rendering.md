# Server Side Rendering

This code example shows how to use Kraken, to implement server-side rendering with a basic templating system.

```c
#include <stdio.h>
#include <stdlib.h>

#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *home_handler(http_req_t *req, http_res_t *res)
{
    placeholder_t placeholders[] = {
        {"{{title}}", "Welcome to the Home Page!"},
        {"{{content}}", "This is some dummy content."},
        {"{{time}}", "10:26 PM"},
    };

    return res_render_template_file("./template.html", placeholders,
                                    NUM_PLACEHOLDERS(placeholders));
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/home", home_handler);

    http_server_listen(server);
    http_server_free(server);
}
```

Kraken supports a basic templating system for server side rendering. You can create a html templates and put placeholders to insert data dynamically. The placeholder variables should be wrapped with `{{ ... }}`. You can more about it in the API reference section.

In this example we are registering a route called `/home`. In the home handler we are using dynamic template system to dynamically insert some data into the template.

```c
placeholder_t placeholders[] = {
  {"{{title}}", "Welcome to the Home Page!"},
  {"{{content}}", "This is some dummy content."},
  {"{{time}}", "10:26 PM"},
};
```

`placeholder_t` is a type defined in kraken. In the example we are creating a array of `placeholder_t` placeholder for out placeholders by mapping each placeholder to some value we want to insert in place of the placeholder.

For the sake of keeping this example short and simple, we are passing some hardcoded values here, but you can pass any values for example you can query a database for some data and pass it here or you can pass the current time.

```c
res_render_template_file("./template.html", placeholders, NUM_PLACEHOLDERS(placeholders));
```

We are using `res_render_template_file` procedure to render a dynamic template file with placeholders. It takes three argument. First is the `path` to the template file. Second is the placeholder values array and the third is the number of items in the placeholder array.

You can pass `3` manually here but if the number of placeholder items change then you need to remember to update it. Kraken comes with a utility macro called `NUM_PLACEHOLDERS` which takes the placeholders array and calculates the number of items in it.
