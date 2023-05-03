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
// #undef PRINT_EXECUTION_TIME
// #define PRINT_EXECUTION_TIME(...) __VA_ARGS__

#define DEBUG_PIN_0 GPIO_NUM_26
#define DEBUG_PIN_1 GPIO_NUM_27
#define DEBUG_PIN_2 GPIO_NUM_14

#define SD_FILE_REFRESH_TIME_US     30000000 // In microseconds
#define FILE_EXTENSION_LEN 5

#define TAG "main"


size_t create_filename_by_date(char *filename, int size)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    return strftime(filename, size, "/%y-%m-%d_%H%M%S.csv", &timeinfo);

}

esp_err_t increment_filename_counter(char *filename, size_t size)
{
    for(int i = 0; i <= 4; i++)
    {
        if (++filename[size -(FILE_EXTENSION_LEN + i)] <= 'f')
        {
            if (filename[size -(FILE_EXTENSION_LEN + i)] == ':')
                filename[size -(FILE_EXTENSION_LEN + i)] = 'a';
            return ESP_OK;
        }
        
        filename[size -(FILE_EXTENSION_LEN + i)] = '0';
    }
    return ESP_FAIL;
}

void app_main(void)
{
    sd_t sd = sd_init();
    if (sd.err != ESP_OK)
        esp_restart();


    esp_sntp_init();
    twai_init();

    // Set timezone to America/SaoPaulo
    setenv("TZ", "<-03>3", 1);
    tzset();
    
    char filename[64];
    size_t filename_size = sizeof(filename) / sizeof(char);

    filename_size = create_filename_by_date(filename, filename_size);

    ESP_LOGI(TAG, "The current date/time in Brazil is: %s", filename);

    FILE *f = sd_open_file(filename);
    if (f == NULL)
        esp_restart();

    twai_message_t message;
    DMA_ATTR static char sd_buffer[37]; 
    struct timeval tv_now;
    uint64_t start,end;
    int write_char;
    int i = 0;
    int message_counter = 0;

    start = esp_timer_get_time();
    for(;;)
    {
        /* Get can message */
        twai_receive_task(&message);

        /* Get timestamp in microseconds */
        gettimeofday(&tv_now, NULL);
        int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;;


        /* Write timestamp and message id into the buffer*/
        write_char = sprintf(sd_buffer, "%llu,%ld,", time_us, message.identifier);

        /* Write the data into the buffer */
        for (int j = 0; j < message.data_length_code; j++)
        {
            write_char += sprintf(sd_buffer + write_char, "%02x",message.data[j]);
        }
        write_char += sprintf(sd_buffer + write_char, "\n");

        /* Write sd buffer into the SD card*/
        fwrite(sd_buffer, 1, write_char, f);
        message_counter++;
        /* Update file */
        // Test 10000
        if ((++i > 10000) || ((esp_timer_get_time() - start) > SD_FILE_REFRESH_TIME_US))
        {

            sd_close_file(f);
            ESP_LOGI(TAG, "%d", message_counter);
            if (message_counter >= 100000)
            {
                ESP_LOGI(TAG, "end of test");
                return;
            }
            
            filename_size = create_filename_by_date(filename, filename_size);
            
            f = sd_open_file(filename);
            if (f == NULL)
                continue;

            end = esp_timer_get_time();
            
            start = esp_timer_get_time();
            i = 0;
        }
    }
    sd_free(f, &sd);

    return;
}
