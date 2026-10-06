---
slug: serving-static-files
title: Serving Static Files with Kraken
authors: [rahul]
tags: [kraken, static-files, tutorial, web-development]
---

Most web applications need to serve static files like HTML pages, CSS stylesheets, JavaScript files, and images. Kraken makes this incredibly easy with its built-in static file serving capability.

<!--truncate-->

## Why Serve Static Files?

Static files are the building blocks of web applications:

- **HTML**: Structure and content of your web pages
- **CSS**: Styling and layout
- **JavaScript**: Client-side interactivity
- **Images**: Photos, logos, and graphics
- **Fonts**: Custom typography
- **Downloads**: PDFs, documents, and other downloadable content

## The `register_static` Function

Kraken provides a simple function to register directories for static file serving:

```c
void register_static(http_server_t *server, char *path);
```

### Parameters

- **`server`**: Your HTTP server instance
- **`path`**: The directory path containing static files (relative to your executable)

## Basic Example

Let's create a simple web server that serves static files:

```c
#include <stdio.h>
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

int main() {
    http_server_t *server = http_server_init(PORT, BACKLOG);
    
    // Register the public directory for static files
    register_static(server, "public");
    
    printf("Serving static files from 'public' directory\n");
    printf("Server running at http://localhost:%d\n", PORT);
    
    http_server_listen(server);
    http_server_free(server);
    
    return 0;
}
```

## Directory Structure

Create a `public` directory with your static files:

```
my-kraken-app/
├── main.c
├── server (executable)
└── public/
    ├── index.html
    ├── about.html
    ├── css/
    │   └── style.css
    ├── js/
    │   └── app.js
    └── images/
        └── logo.png
```

## Creating Static Files

### index.html

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>My Kraken App</title>
    <link rel="stylesheet" href="css/style.css">
</head>
<body>
    <header>
        <img src="images/logo.png" alt="Logo">
        <h1>Welcome to My Kraken App</h1>
    </header>
    
    <nav>
        <a href="/">Home</a>
        <a href="about.html">About</a>
    </nav>
    
    <main>
        <p>This is a static website powered by Kraken!</p>
    </main>
    
    <script src="js/app.js"></script>
</body>
</html>
```

### css/style.css

```css
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: Arial, sans-serif;
    line-height: 1.6;
    color: #333;
    padding: 20px;
}

header {
    text-align: center;
    padding: 2rem;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    border-radius: 10px;
    margin-bottom: 2rem;
}

header img {
    width: 100px;
    height: auto;
}

nav {
    display: flex;
    gap: 1rem;
    margin-bottom: 2rem;
}

nav a {
    padding: 0.5rem 1rem;
    background: #667eea;
    color: white;
    text-decoration: none;
    border-radius: 5px;
    transition: background 0.3s;
}

nav a:hover {
    background: #764ba2;
}
```

### js/app.js

```javascript
console.log('Kraken app loaded!');

document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM fully loaded');
    
    // Add some interactivity
    const main = document.querySelector('main');
    if (main) {
        main.addEventListener('click', () => {
            alert('You clicked the main content!');
        });
    }
});
```

## Multiple Static Directories

Kraken supports registering multiple static file directories:

```c
int main() {
    http_server_t *server = http_server_init(8000, 10);
    
    // Register multiple static directories
    register_static(server, "public");
    register_static(server, "assets");
    register_static(server, "downloads");
    
    http_server_listen(server);
    http_server_free(server);
    
    return 0;
}
```

:::caution Maximum Directories
Kraken currently supports up to 10 static directories (`MAX_STATIC_FILES_PATH`). This is a compile-time constant defined in `http_server.h`.
:::

## Combining Routes and Static Files

You can mix dynamic routes with static file serving:

```c
char *api_handler(http_req_t *req, http_res_t *res) {
    return "{ \"message\": \"Hello from API\" }";
}

int main() {
    http_server_t *server = http_server_init(8000, 10);
    
    // Dynamic routes
    register_route(server, "/api/hello", api_handler);
    
    // Static files
    register_static(server, "public");
    
    printf("Server running on http://localhost:8000\n");
    printf("- Static files: http://localhost:8000/index.html\n");
    printf("- API endpoint: http://localhost:8000/api/hello\n");
    
    http_server_listen(server);
    http_server_free(server);
    
    return 0;
}
```

## How Static File Serving Works

When a request comes in:

1. **Route Check**: Kraken first checks if there's a registered route for the URI
2. **Static File Check**: If no route matches, Kraken looks for a file in registered static directories
3. **File Resolution**: The server attempts to find the file relative to each registered directory
4. **MIME Type Detection**: Kraken detects the file type and sets appropriate headers
5. **Response**: The file contents are sent with the correct `Content-Type` header

## Supported File Types

Kraken automatically detects and serves common file types:

| Extension | Content-Type |
|-----------|--------------|
| `.html` | text/html |
| `.css` | text/css |
| `.js` | application/javascript |
| `.json` | application/json |
| `.png` | image/png |
| `.jpg`, `.jpeg` | image/jpeg |
| `.gif` | image/gif |
| `.svg` | image/svg+xml |
| `.pdf` | application/pdf |
| `.txt` | text/plain |

## Best Practices

### 1. Organize Your Static Files

```
public/
├── index.html
├── css/
│   ├── style.css
│   └── normalize.css
├── js/
│   ├── main.js
│   └── utils.js
├── images/
│   ├── logo.png
│   └── background.jpg
└── fonts/
    └── custom-font.woff2
```

### 2. Use Relative Paths

Always use relative paths in your HTML:

```html
<!-- Good -->
<link rel="stylesheet" href="css/style.css">
<img src="images/logo.png" alt="Logo">

<!-- Avoid absolute paths -->
<link rel="stylesheet" href="/public/css/style.css">
```

### 3. Provide an Index File

Create an `index.html` for the default page when accessing the root directory.

### 4. Security Considerations

:::warning Security
Be careful about what directories you register for static serving. Never register directories containing:
- Source code
- Configuration files
- Database files
- Sensitive data
:::

## Complete Example: Portfolio Website

Here's a complete example serving a simple portfolio website:

```c
#include <stdio.h>
#include "kraken.h"

#define PORT 8000
#define BACKLOG 10

char *contact_handler(http_req_t *req, http_res_t *res) {
    return "{ \"email\": \"contact@example.com\", \"phone\": \"555-0100\" }";
}

int main() {
    http_server_t *server = http_server_init(PORT, BACKLOG);
    
    if (server == NULL) {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }
    
    // API endpoint for contact info
    register_route(server, "/api/contact", contact_handler);
    
    // Static files for the website
    register_static(server, "public");
    
    printf("🦑 Portfolio website running!\n");
    printf("📂 Serving static files from 'public/'\n");
    printf("🌐 Visit: http://localhost:%d\n", PORT);
    
    http_server_listen(server);
    http_server_free(server);
    
    return 0;
}
```

## Troubleshooting

### File Not Found (404)

- Verify the file exists in the static directory
- Check file permissions (must be readable)
- Ensure the path is relative to the executable location

### Wrong Content-Type

- Check the file extension
- Verify Kraken's MIME type mappings support your file type

### Files Not Updating

- Make sure you're clearing browser cache
- Restart the server after adding new files

## What's Next?

Now that you know how to serve static files, explore:

- [Building Your First Server](/blog/building-your-first-server)
- [Understanding Routing](/blog/understanding-routing)
- [Serving Static Files Example](/examples/serving-static-files)
- [HTTP Server API Reference](/api/http-server)

Happy building with Kraken! 🦑
