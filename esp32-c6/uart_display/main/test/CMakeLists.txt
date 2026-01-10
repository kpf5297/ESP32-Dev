idf_component_register(
    SRCS "test_network_debug_task.c"
    INCLUDE_DIRS "."
    REQUIRES unity lvgl__lvgl network_status
)
