/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

#include "esp_log.h"
#include "esp_err.h"

#include "mdns.h"

#include "espfsp_server.h"
#include "udps_handler.h"

#define CONFIG_STREAMER_STACK_SIZE 4096
#define CONFIG_STREAMER_PRIORITY 5

#define CONFIG_STREAMER_CAMERA_PORT_CONTROL 5001
#define CONFIG_STREAMER_CAMERA_PORT_DATA 5002
#define CONFIG_STREAMER_ACCESSOR_PORT_CONTROL 5003
#define CONFIG_STREAMER_ACCESSOR_PORT_DATA 5004

#define CONFIG_STREAMER_FRAME_MAX_LENGTH (100 * 1014)
#define CONFIG_STREAMER_FPS 15
#define CONFIG_STREAMER_BUFFERED_FRAMES 10
#define CONFIG_STREAMER_FRAMES_BEFORE_GET 0

#define CONFIG_STREAMER_MDNS_SERVER_NAME "espfsp_server"

static const char *TAG = "STREAMER_HANDLER";

static espfsp_server_handler_t server_handler = NULL;

static esp_err_t start_mdns_service(const char *hostname)
{
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGI(TAG, "MDNS Init failed: %d\n", err);
        return ESP_FAIL;
    }

    mdns_hostname_set(hostname);
    return ESP_OK;
}

esp_err_t udps_central_init(){
    esp_err_t ret = start_mdns_service(CONFIG_STREAMER_MDNS_SERVER_NAME);
    if (ret != ESP_OK)
    {
        return ret;
    }

    espfsp_server_config_t streamer_config = {
        .client_push_data_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .client_play_data_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .client_push_session_and_control_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .client_play_session_and_control_task_info = {
            .stack_size = CONFIG_STREAMER_STACK_SIZE,
            .task_prio = CONFIG_STREAMER_PRIORITY,
        },
        .client_push_local = {
            .control_port = CONFIG_STREAMER_CAMERA_PORT_CONTROL,
            .data_port = CONFIG_STREAMER_CAMERA_PORT_DATA,
        },
        .client_play_local = {
            .control_port = CONFIG_STREAMER_ACCESSOR_PORT_CONTROL,
            .data_port = CONFIG_STREAMER_ACCESSOR_PORT_DATA,
        },
        .client_push_data_transport = ESPFSP_TRANSPORT_TCP,
        .client_play_data_transport = ESPFSP_TRANSPORT_TCP,
        .frame_config = {
            .frame_max_len = CONFIG_STREAMER_FRAME_MAX_LENGTH,
            .fps = CONFIG_STREAMER_FPS,
            .buffered_fbs = CONFIG_STREAMER_BUFFERED_FRAMES,
            .fb_in_buffer_before_get = CONFIG_STREAMER_FRAMES_BEFORE_GET,
        },
    };

    server_handler  = espfsp_server_init(&streamer_config);
    if (server_handler == NULL) {
        ESP_LOGE(TAG, "Server ESPFSP init failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}
