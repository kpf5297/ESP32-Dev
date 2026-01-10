#include "network_status.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unity.h"

#include "lwip/ip4_addr.h"

#include <string.h>

static void network_status_test_init_once(void)
{
    static bool initialized = false;
    if (initialized) {
        return;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        TEST_FAIL_MESSAGE("esp_netif_init failed");
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        TEST_FAIL_MESSAGE("esp_event_loop_create_default failed");
    }

    TEST_ASSERT_TRUE(network_status_init());

    network_status_task_config_t config;
    network_status_task_get_default_config(&config);
    TEST_ASSERT_TRUE(network_status_start_task(&config));

    initialized = true;
}

TEST_CASE("network_status_get_ipv4 no ip", "[network_status]")
{
    network_status_test_init_once();

    char buffer[NETWORK_STATUS_IPV4_STR_LEN];
    bool connected = network_status_get_ipv4(buffer, sizeof(buffer));

    TEST_ASSERT_FALSE(connected);
    TEST_ASSERT_EQUAL_STRING("", buffer);
}

TEST_CASE("network_status_get_ipv4 valid ip", "[network_status]")
{
    network_status_test_init_once();

    ip_event_got_ip_t event;
    memset(&event, 0, sizeof(event));
    IP4_ADDR(&event.ip_info.ip, 192, 168, 1, 50);

    TEST_ASSERT_EQUAL(ESP_OK,
                      esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                     &event, sizeof(event), portMAX_DELAY));

    vTaskDelay(pdMS_TO_TICKS(50));

    char buffer[NETWORK_STATUS_IPV4_STR_LEN];
    bool connected = network_status_get_ipv4(buffer, sizeof(buffer));

    TEST_ASSERT_TRUE(connected);
    TEST_ASSERT_EQUAL_STRING("192.168.1.50", buffer);
}

TEST_CASE("network_status_get_ipv4 buffer edge cases", "[network_status]")
{
    network_status_test_init_once();

    char buffer[NETWORK_STATUS_IPV4_STR_LEN];
    bool connected = network_status_get_ipv4(NULL, sizeof(buffer));
    TEST_ASSERT_FALSE(connected);

    connected = network_status_get_ipv4(buffer, 0U);
    TEST_ASSERT_FALSE(connected);

    connected = network_status_get_ipv4(buffer, 4U);
    TEST_ASSERT_FALSE(connected);
}
