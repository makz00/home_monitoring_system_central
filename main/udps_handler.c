/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

/*
 * ASSUMPTIONS BEG --------------------------------------------------------------------------------
 * ASSUMPTIONS END --------------------------------------------------------------------------------
 *
 * TODO BEG ---------------------------------------------------------------------------------------
 * - Configuration options to add in 'menuconfig'/Kconfig file
 * TODO END ---------------------------------------------------------------------------------------
 */

#include "driver/gpio.h"
#include "esp_log.h"
#include "udp_streamer_handler.h"
#include "udps_handler.h"

#define CONFIG_UDP_STREAMER_HANDLER_HOST "192.168.8.146"
#define CONFIG_UDP_STREAMER_HANDLER_PORT_CONTROL 5001
#define CONFIG_UDP_STREAMER_HANDLER_PORT_DATA 5002
#define CONFIG_UDP_STREAMER_HANDLER_BUFFERED_FRAMES 10

static const char *TAG = "UDPS_HANDLER";

static udps_config_t udps_config = {
    .host = CONFIG_UDP_STREAMER_HANDLER_HOST,
    .control_port = CONFIG_UDP_STREAMER_HANDLER_PORT_CONTROL,
    .data_port = CONFIG_UDP_STREAMER_HANDLER_PORT_DATA,
    .buffered_fbs = CONFIG_UDP_STREAMER_HANDLER_BUFFERED_FRAMES
};

esp_err_t udps_init(){
    esp_err_t err = udp_streamer_handler_init(&udps_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UDP streamer handler Init Failed");
        return err;
    }

    return ESP_OK;
}
