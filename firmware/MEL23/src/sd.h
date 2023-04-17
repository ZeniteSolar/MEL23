#ifndef SD_H
#define SD_H

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

typedef struct
{
    sdmmc_card_t *card;
    sdmmc_host_t host;
    esp_err_t err;
} sd_t;

sd_t sd_init(void);

FILE *sd_open_file(const char *path);

esp_err_t sd_close_file(FILE *f);

esp_err_t sd_free(FILE *f, sd_t *sd);

esp_err_t sd_write_file(FILE *f, char *data);

#endif