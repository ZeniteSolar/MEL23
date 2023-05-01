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
#include "esp_timer.h"


#define PRINT_EXECUTION_TIME(...)          \
    start = esp_timer_get_time(); \
    __VA_ARGS__;                           \
    end = esp_timer_get_time();   \
    ESP_LOGI(TAG, "line %d, took: %llu microseconds", __LINE__,(end - start))
#undef PRINT_EXECUTION_TIME
#define PRINT_EXECUTION_TIME(...) __VA_ARGS__

#define DEBUG_PIN_0 GPIO_NUM_26
#define DEBUG_PIN_1 GPIO_NUM_27
#define DEBUG_PIN_2 GPIO_NUM_14

#define TAG "main"

void app_main(void)
{
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
    twai_init();

    // Set timezone to America/SaoPaulo
    setenv("TZ", "<-03>3", 1);
    tzset();
    time_t now;
    struct tm timeinfo;
    char filename[64];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(filename, sizeof(filename), "/%y-%m-%d_%H%M%S.txt", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Brazil is: %s", filename);

    FILE *f;
    twai_message_t message;
    int i = 0;
    f = sd_open_file(filename);
    if (f == NULL)
        esp_restart();
    message.identifier = 10;
    uint8_t data[] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
    *(uint32_t *)message.data = *(uint32_t *)data;
    message.data_length_code = 8;
    uint64_t start,end;
    DMA_ATTR static char sd_buffer[43]; 
    while (1)
    {

        struct timeval tv_now;
        // twai_receive_task(&message);
        
        PRINT_EXECUTION_TIME(gettimeofday(&tv_now, NULL));
        
        PRINT_EXECUTION_TIME(int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;);

        PRINT_EXECUTION_TIME(sprintf(sd_buffer, "%llu,%ld,%02x%02x%02x%02x%02x%02x%02x%02x\n",
        time_us,
        message.identifier, 
        message.data[0], 
        message.data[0], 
        message.data[0], 
        message.data[0], 
        message.data[0], 
        message.data[0], 
        message.data[0], 
        message.data[0]);
        fwrite(sd_buffer, 1, sizeof(sd_buffer) / sizeof(char), f));

        // PRINT_EXECUTION_TIME(fprintf(f, "%llu,%ld,%02x%02x%02x%02x%02x%02x%02x%02x\n",
        // time_us,
        // message.identifier, 
        // message.data[0], 
        // message.data[0], 
        // message.data[0], 
        // message.data[0], 
        // message.data[0], 
        // message.data[0], 
        // message.data[0], 
        // message.data[0]););
        // PRINT_EXECUTION_TIME(fprintf(f, "%llu,%ld,", time_us, message.identifier););
        // PRINT_EXECUTION_TIME(for (int i = 0; i < message.data_length_code; i++)
        // {
        //     fprintf(f, "%02x", message.data[i]);
        // }
        // );
        // PRINT_EXECUTION_TIME(fprintf(f, "\n"););

        if (++i > 1000)
        {
            i = 0;
            PRINT_EXECUTION_TIME(
            sd_close_file(f);
            f = sd_open_file(filename);
            if (f == NULL)
                continue;
            );
        }
    }
    sd_free(f, &sd);

    return;
}
