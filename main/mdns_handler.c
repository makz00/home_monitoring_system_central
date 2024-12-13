/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

#include "esp_err.h"
#include "esp_log.h"
#include "mdns.h"

#include "mdns_handler.h"

#define CONFIG_CAMERA_CONTROL_PORT 5001
#define CONFIG_CAMERA_SENDER_PORT 5002

static const char *TAG = "MDNS_HANDLER";

esp_err_t mdns_service_init()
{
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(TAG, "MDNS Init failed: %d\n", err);
        return err;
    }

    return ESP_OK;
}
