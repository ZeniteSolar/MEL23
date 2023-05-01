#ifndef UTILS_H
#define UTILS_H

#define ESP_RETURN_ON_ERROR_AND_PRINT(x, tag,...) \
    do                                                  \
    {                                                   \
        ESP_ERROR_CHECK_WITHOUT_ABORT(x);               \
        esp_err_t err_rc_ = (x);                        \
        if (unlikely(err_rc_ != ESP_OK))                \
        {                                               \
            ESP_LOGE(tag, __VA_ARGS__);            \
            return x;                                   \
        }                                               \
    } while (0)

#endif