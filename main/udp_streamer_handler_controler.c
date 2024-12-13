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

#include <arpa/inet.h>
#include "esp_netif.h"
#include <netdb.h>
#include <sys/socket.h>
#include "mdns.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_netif_ip_addr.h"

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "udp_streamer_handler.h"
#include "udp_streamer_handler_controler.h"
#include "udp_streamer_handler_types.h"

static const char *TAG = "UDP_STREAMER_COMPONENT_CONTROLER";

static const char *PAYLOAD_HELLO = "HELLO";
static const char *PAYLOAD_READY = "READY";

typedef enum
{
    CONTROL_STATE_HELLO,
    CONTROL_STATE_READY,
    CONTROL_STATE_IDLE,
} control_state_t;

typedef enum
{
    CONTROL_STATE_RET_OK,
    CONTROL_STATE_RET_FAIL,
    CONTROL_STATE_RET_ERR,
} control_state_ret_t;

static control_state_ret_t process_control_handle_hello(int sock)
{
    int ret = send(sock, PAYLOAD_HELLO, strlen(PAYLOAD_HELLO), 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return CONTROL_STATE_RET_FAIL;
    }

    if (ret != strlen(PAYLOAD_HELLO))
    {
        ESP_LOGE(TAG, "Not send whole message");
        return CONTROL_STATE_RET_FAIL;
    }

    return CONTROL_STATE_RET_OK;
}

static control_state_ret_t process_control_handle_ready(int sock)
{
    int ret = send(sock, PAYLOAD_READY, strlen(PAYLOAD_READY), 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return CONTROL_STATE_RET_FAIL;
    }

    if (ret != strlen(PAYLOAD_READY))
    {
        ESP_LOGE(TAG, "Not send whole message");
        return CONTROL_STATE_RET_FAIL;
    }

    return CONTROL_STATE_RET_OK;
}

static void process_control_connection(int sock)
{
    control_state_ret_t ret;
    control_state_t state;

    state = CONTROL_STATE_HELLO;

    while (1)
    {
        switch (state)
        {
        case CONTROL_STATE_HELLO:
            ret = process_control_handle_hello(sock);

            if (ret == CONTROL_STATE_RET_OK)
            {
                ESP_LOGI(TAG, "State HELLO reached");
                state = CONTROL_STATE_READY;
            }
            break;

        case CONTROL_STATE_READY:
            ret = process_control_handle_ready(sock);

            if (ret == CONTROL_STATE_RET_OK)
            {
                ESP_LOGI(TAG, "State READY reached");
                state = CONTROL_STATE_IDLE;
            }
            else if (ret == CONTROL_STATE_RET_FAIL)
            {
                ESP_LOGI(TAG, "State READY failed. Reapeting");
                state = CONTROL_STATE_HELLO;
            }
            break;

        case CONTROL_STATE_IDLE:
            ESP_LOGI(TAG, "State IDLE reached");
            vTaskSuspend(NULL);
            break;
        }

        if (ret == CONTROL_STATE_RET_ERR)
        {
            ESP_LOGE(TAG, "Control protocol end with critical error");
            break;
        }
    }
}

static int find_camera_service(const char *hostname, struct esp_ip4_addr *addr)
{
    int wait_time_in_milisec = 2000;

    while (1)
    {
        esp_err_t ret = mdns_query_a(hostname, wait_time_in_milisec, addr);
        if(ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Successfully found host IP for %s", hostname);
            break;
        }
        else if(ret == ESP_ERR_NOT_FOUND){
            ESP_LOGI(TAG, "Host '%s' was not found ...", hostname);
            continue;
        }
        else
        {
            ESP_LOGE(TAG, "MDNS unknown error");
            return -1;
        }
    }

    return 1;
}

void udp_streamer_control_task(void *pvParameters)
{
    const udps_config_t *config = (udps_config_t *)pvParameters;

    struct esp_ip4_addr addr;
    int ret = find_camera_service(config->hostname, &addr);
    if (ret < 0)
    {
        vTaskDelete(NULL);
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = addr.addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(config->control_port);

    char host_address_str[128];
    const char *s = inet_ntop(AF_INET, &dest_addr.sin_addr.s_addr, host_address_str, sizeof(host_address_str));
    if (s == NULL)
    {
        ESP_LOGE(TAG, "Unable to convert address from bin to text format : errno %d", errno);
        vTaskDelete(NULL);
    }

    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    while (1)
    {
        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            continue;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_address_str, config->control_port);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            close(sock);
            continue;
        }
        ESP_LOGI(TAG, "Successfully connected");

        process_control_connection(sock);

        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }

    vTaskDelete(NULL);
}
