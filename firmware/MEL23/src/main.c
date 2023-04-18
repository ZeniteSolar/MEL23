#include "sd.h"
#include "stdio.h"
#include "twai.h"
#include "esp_random.h"
#include "time.h"
#include <sys/time.h>
#include <unistd.h>
#include "wifi.h"
#include "sntp.h"
#include <string.h>
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

    // Set timezone to America/SaoPaulo
    setenv("TZ", "<-03>3", 1);
    tzset();
    time_t now;
    struct tm timeinfo;
    char filename[64];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(filename, sizeof(filename), "/%y-%m-%d-(%H_%M_%S).txt", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Brazil is: %s", filename);

    FILE *f;
    while (1)
    {
        f = sd_open_file(filename);
        if (f == NULL)
            return;

        struct timeval tv_now;

        gettimeofday(&tv_now, NULL);
        int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        fprintf(f, "%llu,%ld,%llu\n", time_us, message.identifier, *(uint64_t *)message.data);
        ESP_LOGI(TAG, "timestamp: %llu", time_us);
        sd_close_file(f);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    sd_free(f, &sd);

    return;
}
