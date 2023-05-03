#include "twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utils.h"

#define PIN_NUM_RX CONFIG_TWAI_RX
#define PIN_NUM_TX CONFIG_TWAI_TX

#define TAG "twai"

esp_err_t twai_init(void)
{
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(PIN_NUM_RX, PIN_NUM_TX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Driver installed\n");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to install driver\n");
        return ESP_FAIL;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        ESP_LOGI(TAG, "Driver started\n");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start driver\n");
        return ESP_FAIL;
    }


    return ESP_OK;
}

esp_err_t twai_receive_task(twai_message_t *message)
{
    return twai_receive(message, pdMS_TO_TICKS(100));
}