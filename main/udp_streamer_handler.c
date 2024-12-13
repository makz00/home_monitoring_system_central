/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

/*
 * ASSUMPTIONS BEG --------------------------------------------------------------------------------
 * - One format of pictures supported - HD compressed with JPEG
 * - Constant size for one Frame Buffer, so allocation can take place before messagas arrived
 * ASSUMPTIONS END --------------------------------------------------------------------------------
 *
 * TODO BEG ---------------------------------------------------------------------------------------
 * - Configuration options to add in 'menuconfig'/Kconfig file
 * TODO END ---------------------------------------------------------------------------------------
 */


#define UDPS_RECEIVER_PRIORITY 18
#define UDPS_CONTROL_PRIORITY 5

#include "esp_err.h"
#include "esp_log.h"
#include "sys/time.h"
#include <stdint.h>

#include <stdalign.h>
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "udp_streamer_handler.h"
#include "udp_streamer_handler_controler.h"
#include "udp_streamer_handler_receiver.h"
#include "udp_streamer_handler_types.h"

static const char *TAG = "UDP_STREAMER_COMPONENT";

udps_state_t *s_state = NULL;

QueueHandle_t xQueue;

static udps_fb_t *s_fb = NULL;
static udps_message_assembly_t *s_ass = NULL;

esp_err_t udp_streamer_handler_buffers_init(int buffered_fbs)
{
    s_state = (udps_state_t *)heap_caps_malloc(sizeof(udps_state_t), MALLOC_CAP_DEFAULT);

    if (!s_state)
    {
        ESP_LOGE(TAG, "Cannot allocate memory for s_state");
        return ESP_FAIL;
    }

    s_state->fbs_messages_buf = (udps_message_assembly_t *)heap_caps_aligned_calloc(
        alignof(udps_message_assembly_t), 1, buffered_fbs * sizeof(udps_message_assembly_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);

    if (!s_state->fbs_messages_buf)
    {
        ESP_LOGE(TAG, "Cannot allocate memory for s_state->fbs_messages");
        return ESP_FAIL;
    }

    s_state->fbs_messages_buf_size = buffered_fbs;
    s_state->fbs_message_oldest = NULL;

    size_t alloc_size;

    // HD resolution: 1280*720. In esp_cam it is diviced by 5. Here Resolution is diviced by 3;
    // alloc_size = 307200;

    alloc_size = 100000;

    // It is safe to assume that this size packet will not be fragmented
    // size_t max_udp_packet_size = 512;

    // It is 307200 / 512
    // int number_of_messages_assembly = 600;

    for (int i = 0; i < buffered_fbs; i++)
    {
        s_state->fbs_messages_buf[i].buf = (uint8_t *)heap_caps_malloc(alloc_size * sizeof(uint8_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);

        if (!s_state->fbs_messages_buf[i].buf)
        {
            ESP_LOGE(TAG, "Cannot allocate memory for s_state->fbs_messages[i].buf for i=%d", i);
            return ESP_FAIL;
        }

        s_state->fbs_messages_buf[i].bits = MSG_ASS_PRODUCER_OWNED_VAL | MSG_ASS_FREE_VAL;
    }

    s_fb = (udps_fb_t *)heap_caps_malloc(sizeof(udps_fb_t), MALLOC_CAP_DEFAULT);

    if (!s_fb)
    {
        ESP_LOGE(TAG, "Cannot allocate memory for fb");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t udp_streamer_handler_queue_init(int buffered_fbs)
{
    xQueue = xQueueCreate(buffered_fbs, sizeof(udps_message_assembly_t*));

    if (xQueue == NULL)
    {
        ESP_LOGE(TAG, "Cannot initialize message assembly queue");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t udp_streamer_handler_start_receiver(int data_port)
{
    BaseType_t xStatus;

    xStatus = xTaskCreatePinnedToCore(udp_streamer_receiver_task, "udp_streamer_receiver_task", 4096, (void *)data_port, UDPS_RECEIVER_PRIORITY, &s_state->receiver_task_handle, 1);
    if (xStatus != pdPASS)
    {
        ESP_LOGE(TAG, "Could not start receiver task!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t udp_streamer_handler_connect_camera(const udps_config_t *config)
{
    BaseType_t xStatus;

    xStatus = xTaskCreate(udp_streamer_control_task, "udp_streamer_control_task", 4096, (void *)config, UDPS_CONTROL_PRIORITY, &s_state->control_task_handle);
    if (xStatus != pdPASS)
    {
        ESP_LOGE(TAG, "Could not start receiver task!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t udp_streamer_handler_init(const udps_config_t *config)
{
    esp_err_t ret;

    ret = udp_streamer_handler_buffers_init(config->buffered_fbs);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Buffer initialization failed");
        return ret;
    }

    ret = udp_streamer_handler_queue_init(config->buffered_fbs);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Queue initialization failed");
        return ret;
    }

    ret = udp_streamer_handler_start_receiver(config->data_port);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Receiver start failed");
        return ret;
    }

    ret = udp_streamer_handler_connect_camera(config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Connection initialization failed");
        return ret;
    }

    return ret;
}

esp_err_t udp_streamer_handler_deinit(void)
{
    ESP_LOGE(TAG, "NOT IMPLEMENTED!");

    return ESP_FAIL;
}

#define FB_GET_TIMEOUT (4000 / portTICK_PERIOD_MS)

udps_fb_t *udp_streamer_handler_fb_get(void)
{
    if (s_fb == NULL)
    {
        ESP_LOGE(TAG, "Frame buffer has to be allocated first with init function");
        return NULL;
    }

    BaseType_t xStatus = xQueueReceive(xQueue, &s_ass, FB_GET_TIMEOUT);
    if (xStatus != pdTRUE)
    {
        ESP_LOGE(TAG, "Cannot read assembly from queue");
        return NULL;
    }

    s_fb->len = s_ass->len;
    s_fb->width = s_ass->width;
    s_fb->height = s_ass->height;
    s_fb->timestamp = s_ass->timestamp;
    s_fb->buf = s_ass->buf;

    return s_fb;
}

void udp_streamer_handler_fb_return(udps_fb_t *fb)
{
    if (s_fb == NULL)
    {
        ESP_LOGE(TAG, "Frame buffer has to be allocated first with init function");
        return;
    }

    if (s_ass == NULL)
    {
        ESP_LOGE(TAG, "Frame buffer has to be took first with fb get function");
        return;
    }

    s_ass->bits = MSG_ASS_PRODUCER_OWNED_VAL | MSG_ASS_FREE_VAL;
}
