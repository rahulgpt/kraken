---
slug: building-your-first-server
title: Building Your First HTTP Server with Kraken
authors: rahul
tags: [kraken, tutorial, http-server, getting-started]
---

In this tutorial, we'll walk through building your first HTTP server using Kraken. By the end, you'll have a working web server that can handle requests and serve responses.

<!--truncate-->

## Prerequisites

Before we begin, make sure you have:

- GCC or another C compiler installed
- Basic knowledge of C programming
- Kraken installed on your system

## Step 1: Setting Up Your Project

First, create a new directory for your project and create a `main.c` file:

```bash
mkdir my-kraken-app
cd my-kraken-app
touch main.c
```

## Step 2: Include Kraken

At the top of your `main.c` file, include the necessary headers:

```c
#include <stdio.h>
#include <stdlib.h>
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10
```

The `PORT` defines where your server will listen, and `BACKLOG` defines the maximum number of pending connections.

## Step 3: Create Your First Route Handler

Route handlers are functions that process incoming requests and return responses. Let's create a simple handler:

```c
char *index_handler(http_req_t *req, http_res_t *res)
{
    return "Welcome to My Kraken Server!";
}
```

This handler receives two parameters:
- `http_req_t *req`: Contains information about the incoming request
- `http_res_t *res`: Allows you to configure the response

For simple text responses, just return a string!

## Step 4: Initialize the Server

Now let's set up the server in the `main` function:

```c
int main()
{
    // Initialize the server
    http_server_t *server = http_server_init(PORT, BACKLOG);
    
    if (server == NULL) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    printf("Server initialized on port %d\n", PORT);
    
    // Register your route
    register_route(server, "/", index_handler);
    
    // Start listening for connections
    printf("Server listening on http://localhost:%d\n", PORT);
    http_server_listen(server);
    
    // Clean up
    http_server_free(server);
    
    return 0;
}
```

## Step 5: Compile and Run

Compile your application:

```bash
gcc -o server main.c -lkraken -lpthread
```

Run your server:

```bash
./server
```

You should see:
```
Server initialized on port 8000
Server listening on http://localhost:8000
```

## Step 6: Test Your Server

Open your browser and navigate to `http://localhost:8000`. You should see "Welcome to My Kraken Server!" displayed on the page.

Alternatively, use curl:

```bash
curl http://localhost:8000
```

## Adding More Routes

You can add multiple routes to handle different endpoints:

```c
char *about_handler(http_req_t *req, http_res_t *res)
{
    return "This is a Kraken-powered server!";
}

char *contact_handler(http_req_t *req, http_res_t *res)
{
    return "Contact us at: contact@example.com";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);
    
    register_route(server, "/", index_handler);
    register_route(server, "/about", about_handler);
    register_route(server, "/contact", contact_handler);
    
    http_server_listen(server);
    http_server_free(server);
    
    return 0;
}
```

## Understanding What Happens

When a request comes in:

1. **Connection Accepted**: The server accepts the incoming TCP connection
2. **Request Parsed**: The HTTP request is parsed into an `http_req_t` structure
3. **Route Matched**: The server looks up the handler for the requested URI
4. **Handler Executed**: Your handler function is called with the request and response objects
5. **Response Sent**: The return value is sent back to the client
6. **Connection Closed**: The connection is closed (HTTP/1.0 behavior)

## Next Steps

Now that you have a basic server running, you can:

- Learn about [serving static files](/blog/serving-static-files)
- Explore the [HTTP Request API](/api/http-req)
- Dive into [HTTP Response customization](/api/http-res)
- Check out more [examples](/examples/creating-a-endpoint)

## Complete Code

Here's the complete `main.c` file:

```c
#include <stdio.h>
#include <stdlib.h>
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *index_handler(http_req_t *req, http_res_t *res)
{
    return "Welcome to My Kraken Server!";
}

char *about_handler(http_req_t *req, http_res_t *res)
{
    return "This is a Kraken-powered server!";
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);
    
    if (server == NULL) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    register_route(server, "/", index_handler);
    register_route(server, "/about", about_handler);
    
    printf("Server listening on http://localhost:%d\n", PORT);
    http_server_listen(server);
    
    http_server_free(server);
    
    return 0;
}
```

Happy coding with Kraken! 🦑
