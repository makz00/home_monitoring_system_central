/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

#include "esp_log.h"
#include "esp_err.h"

#include "streamer_central.h"
#include "udps_handler.h"

#define CONFIG_STREAMER_STACK_SIZE 4096
#define CONFIG_STREAMER_PRIORITY 5

#define CONFIG_STREAMER_CAMERA_PORT_CONTROL 5001
#define CONFIG_STREAMER_CAMERA_PORT_DATA 5002
#define CONFIG_STREAMER_ACCESSOR_PORT_CONTROL 5003
#define CONFIG_STREAMER_ACCESSOR_PORT_DATA 5004

#define CONFIG_STREAMER_MDNS_SERVER_NAME "central_server"

#define CONFIG_STREAMER_FPS 15
#define CONFIG_STREAMER_FRAME_MAX_LENGTH (100 * 1014)
#define CONFIG_STREAMER_BUFFERED_FRAMES 10

static const char *TAG = "STREAMER_HANDLER";

esp_err_t udps_central_init(){
    streamer_config_t streamer_config = {
        .data_receive_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .data_send_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .camera_control_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .client_control_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .camera_local_ports = {
            .control_port = CONFIG_STREAMER_CAMERA_PORT_CONTROL,
            .data_port = CONFIG_STREAMER_CAMERA_PORT_DATA,
        },
        .client_local_ports = {
            .control_port = CONFIG_STREAMER_ACCESSOR_PORT_CONTROL,
            .data_port = CONFIG_STREAMER_ACCESSOR_PORT_DATA,
        },
        .trans = STREAMER_TRANSPORT_UDP,
        .buffered_fbs = CONFIG_STREAMER_BUFFERED_FRAMES,
        .frame_max_len = CONFIG_STREAMER_FRAME_MAX_LENGTH,
        .fps = CONFIG_STREAMER_FPS,
        .server_mdns_name = CONFIG_STREAMER_MDNS_SERVER_NAME,
    };

    esp_err_t err = streamer_central_init(&streamer_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UDP streamer handler central init failed");
        return err;
    }

    return ESP_OK;
}
