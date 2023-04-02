#include "owl/utils/log.h"
#include "src/http_req.h"
#include "src/http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROTOCOL 0
#define PORT 8000
#define BACKLOG 10

int main()
{
    http_server_t *server = http_server_init(PORT, BACKLOG);
    http_server_listen(server);
}