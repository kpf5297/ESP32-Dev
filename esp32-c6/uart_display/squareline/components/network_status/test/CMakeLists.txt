idf_component_register(
    SRCS "test_network_status.c"
    INCLUDE_DIRS "."
    REQUIRES network_status unity esp_event esp_netif
)
