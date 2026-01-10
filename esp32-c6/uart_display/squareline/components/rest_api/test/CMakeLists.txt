idf_component_register(
    SRCS "test_rest_api.c" "test_rest_api_integration.c"
    INCLUDE_DIRS "."
    REQUIRES rest_api system_snapshot unity esp_http_client esp_netif
)
