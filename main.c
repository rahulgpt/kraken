#include "owl/utils/log.h"
#include "src/http_req.h"
#include "src/http_res.h"
#include "src/http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT 8000
#define BACKLOG 10

char *home_handler(http_req_t *req, http_res_t *res)
{
    res_status(res, 200);
    res_content_type(res, "text/html");

    placeholder_t placeholders[] = {
        {"{{title}}", "Welcome to the Home Page!"},
        {"{{content}}", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc eget est nibh. Sed auctor, diam elementum interdum vehicula, lectus urna euismod arcu, ut feugiat odio arcu et dui. Cras id sodales neque, accumsan luctus ante. Nam sed ullamcorper enim. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Curabitur rhoncus nulla at pharetra feugiat. Mauris vel odio erat. Donec finibus turpis sit amet suscipit vestibulum. Donec at urna elementum, pellentesque ante eget, congue nisi. Praesent enim lectus, facilisis sit amet turpis quis, rhoncus venenatis turpis. Donec porta tellus in mauris consequat, vitae cursus diam commodo. Fusce ullamcorper luctus sagittis. Morbi lacus odio, accumsan non rutrum non, consectetur vel tortor. "},
        {"{{something1}}", "10:26 PM"},
    };

    return res_render_template_file("./template.html", placeholders, NUM_PLACEHOLDERS(placeholders));
}

char *about_handler(http_req_t *req, http_res_t *res)
{
    return "<h1>About Page</h1><a href=\"/home\">Home</a>";
}

char *index_handler(http_req_t *req, http_res_t *res)
{
    return res_render_static_file("./react/react.html");
}

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);

    register_route(server, "/home", home_handler);
    register_route(server, "/about", about_handler);
    register_route(server, "/", index_handler);

    register_static(server, "public");
    register_static(server, "react");
    register_static(server, "next");

    http_server_listen(server);
    http_server_free(server);
}