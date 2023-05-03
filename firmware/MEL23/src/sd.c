/* SD card and FAT filesystem example.
   This example uses SPI peripheral to communicate with SD card.
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "sd.h"

#define EXAMPLE_MAX_CHAR_SIZE 64

static const char *TAG = "sd_card";

#define MOUNT_POINT "/sd"

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration"
// menu. You can also change the pin assignments here by changing the following
// 4 lines.
#define PIN_NUM_MISO CONFIG_SD_PIN_MISO
#define PIN_NUM_MOSI CONFIG_SD_PIN_MOSI
#define PIN_NUM_CLK CONFIG_SD_PIN_CLK
#define PIN_NUM_CS CONFIG_SD_PIN_CS



sd_t sd_init(void)
{
    esp_err_t ret;
    sd_t sd;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz -
    // 20MHz for SDSPI) Example: for fixed frequency of 10MHz, use
    // host.max_freq_khz = 10000;
    sd.host = (sdmmc_host_t)SDSPI_HOST_DEFAULT();
    
    sd.host.max_freq_khz = 10000;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };
    ret = spi_bus_initialize(sd.host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        sd.err = ESP_FAIL;
        return sd;
    }

    // This initializes the slot without card detect (CD) and write protect (WP)
    // signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if your board
    // has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = sd.host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &sd.host, &slot_config, &mount_config,
                                  &sd.card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the "
                          "CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG,
                     "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        sd.err = ESP_FAIL;
        return sd;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sd.card);

    sd.err = ESP_OK;
    return sd;
}

FILE *sd_open_file(const char *path)
{

    char file_path[EXAMPLE_MAX_CHAR_SIZE] = MOUNT_POINT;
    strcat(file_path, path);
    ESP_LOGI(TAG, "Opening file %s", file_path);
    FILE *f = fopen(file_path, "a");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        ESP_ERROR_CHECK(ESP_FAIL);
        return NULL;
    }
    ESP_LOGI(TAG, "Opened file %s", file_path);

    return f;
}

esp_err_t sd_close_file(FILE *f)
{

    fclose(f);
    ESP_LOGI(TAG, "File closed");
    return ESP_OK;
}

esp_err_t sd_free(FILE *f, sd_t *sd)
{

    if (f != NULL)
        sd_close_file(f);
    // All done, unmount partition and disable SPI peripheral
    const char mount_point[] = MOUNT_POINT;
    esp_vfs_fat_sdcard_unmount(mount_point, sd->card);
    ESP_LOGI(TAG, "Card unmounted");

    // deinitialize the bus after all devices are removed
    spi_bus_free(sd->host.slot);
    return ESP_OK;
}
