#include "sd.h"
#include "stdio.h"
#include "twai.h"
#include "esp_random.h"
#include "time.h"
#include "wifi.h"
#include "sntp.h"
// Include FreeRTOS for delay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define DEBUG_PIN_0 GPIO_NUM_26
#define DEBUG_PIN_1 GPIO_NUM_27
#define DEBUG_PIN_2 GPIO_NUM_14

#define TAG "main"

void app_main(void)
{
    volatile twai_message_t message = {
        .identifier = 0x10,
        .data_length_code = 8,
        .data = {0xff, 0xfe, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7},
    };
    sd_t sd = sd_init();
    if (sd.err != ESP_OK)
        return;

    gpio_reset_pin(DEBUG_PIN_0);
    gpio_set_direction(DEBUG_PIN_0, GPIO_MODE_OUTPUT);
    gpio_set_level(DEBUG_PIN_0, 0);

    gpio_reset_pin(DEBUG_PIN_1);
    gpio_set_direction(DEBUG_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_level(DEBUG_PIN_1, 0);

    gpio_reset_pin(DEBUG_PIN_2);
    gpio_set_direction(DEBUG_PIN_2, GPIO_MODE_OUTPUT);

    esp_sntp_init();

    char filename[64];
    localtime_r(&now, &timeinfo);
    strftime(filename, sizeof(filename), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Brazil is: %s", filename);

    // Set timezone to America/SaoPaulo
    setenv("TZ", "<-03>3", 1);
    tzset();
    FILE *f;
    while (1)
    {
        f = sd_open_file("/batatao.txt");
        if (f == NULL)
            return;
        fprintf(f, "%llu,%ld,%llu\n", time(NULL), message.identifier, *(uint64_t *)message.data);
        ESP_LOGI(TAG, "timestamp: %llu", time(NULL));
        sd_close_file(f);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    sd_free(f, &sd);

    return;
}
