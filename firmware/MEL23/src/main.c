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
#define CAN_SD_MESSAGE_SIZE 37

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

typedef struct 
{
    char *head;
    char *consumed;
    char *tail;
}sd_buffer_t;


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

    DMA_ATTR static char buffer[CAN_SD_MESSAGE_SIZE * 3072]; 
    sd_buffer_t sd_buffer = {
        .head = buffer,
        .consumed = buffer,
        .tail = buffer + sizeof(buffer) / sizeof(char),
    };

    struct timeval tv_now;
    uint64_t start,end;
    int i = 0;
    int message_counter = 0;

    start = esp_timer_get_time();
    for(;;)
    {
        /* Get can message */
        if (twai_receive_task(&message) == ESP_OK)
        {
            /* Get timestamp in microseconds */
            gettimeofday(&tv_now, NULL);
            int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;;

            /* Write timestamp and message id into the buffer*/
            sd_buffer.consumed += sprintf(sd_buffer.consumed, "%llu,%lx,", time_us, message.identifier);

            /* Write the data into the buffer */
            for (int j = 0; j < message.data_length_code; j++)
            {
                sd_buffer.consumed += sprintf(sd_buffer.consumed, "%02x",message.data[j]);
            }
            sd_buffer.consumed += sprintf(sd_buffer.consumed, "\n");

            if (sd_buffer.consumed + CAN_SD_MESSAGE_SIZE > sd_buffer.tail)
            {
                
                /* Write sd buffer into the SD card*/
                fwrite(sd_buffer.head, 1, sd_buffer.consumed - sd_buffer.head , f);
                /* Clear buffer index */
                sd_buffer.consumed = sd_buffer.head;
            }
            message_counter++;
        }

        /* Update file */
        // Test 10000
        if ((++i > 300000) || ((esp_timer_get_time() - start) > SD_FILE_REFRESH_TIME_US))
        {
            /* Write sd buffer into the SD card*/
            fwrite(sd_buffer.head, 1, sd_buffer.consumed - sd_buffer.head , f);
            /* Clear buffer index */
            sd_buffer.consumed = sd_buffer.head;

            sd_close_file(f);
            printf("%d\n", message_counter);

            
            create_filename_by_date(filename, filename_size);

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
