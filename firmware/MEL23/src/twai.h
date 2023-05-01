#ifndef TWAI_H
#define TWAI_H

#include "driver/twai.h"

esp_err_t twai_init(void);

esp_err_t twai_receive_task(twai_message_t *message);

#endif