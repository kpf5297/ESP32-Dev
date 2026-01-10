idf_component_register(
    SRCS "rest_api.c" "rest_routes.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server system_snapshot
)
