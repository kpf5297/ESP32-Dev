#pragma once

#include "esp_http_server.h"
#include <stdbool.h>

bool rest_routes_register(httpd_handle_t server);

