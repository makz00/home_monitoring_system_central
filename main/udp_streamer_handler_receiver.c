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

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "udp_streamer_handler_receiver.h"
#include "udp_streamer_handler_receiver_message_processor.h"
#include "udp_streamer_handler_types.h"

static const char *TAG = "UDP_STREAMER_COMPONENT_RECEIVER";

static int receive_whole_message(int sock, char *rx_buffer, int rx_buffer_len)
{
    int accepted_error_count = 5;
    int received_bytes = 0;

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);

    while (received_bytes < rx_buffer_len)
    {
        int len = recvfrom(sock, rx_buffer, rx_buffer_len - received_bytes, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len < 0)
        {
            ESP_LOGE(TAG, "Receiving message failed: errno %d", errno);
            if (accepted_error_count-- > 0)
            {
                continue;
            }
            else
            {
                ESP_LOGE(TAG, "Accepted error count reached. Message will not be received");
                return -1;
            }
        }
        else if (len == 0)
        {
            ESP_LOGE(TAG, "Sender side closed connection");
            return -1;
        }
        else
        {
            received_bytes += len;
            rx_buffer += len;
        }
    }

    return 1;
}

// static void punch_hole_in_nat(int sock)
// {
//     uint8_t bullet = 0;
//     while (sendto(sock, &bullet, sizeof(bullet), 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr)) > 0) {}
// }

static void process_receiver_connection(int sock)
{
    char rx_buffer[sizeof(udps_message_t)];

    while (1)
    {
        int ret = receive_whole_message(sock, rx_buffer, sizeof(udps_message_t));
        if (ret > 0)
        {
            // ESP_LOGI(
            //     TAG,
            //     "Received msg part: %d/%d for timestamp: sek: %lld, usek: %ld",
            //     ((udps_message_t *)rx_buffer)->msg_number,
            //     ((udps_message_t *)rx_buffer)->msg_total,
            //     ((udps_message_t *)rx_buffer)->timestamp.tv_sec,
            //     ((udps_message_t *)rx_buffer)->timestamp.tv_usec);

            process_message((udps_message_t *)rx_buffer);
        }
        else
        {
            ESP_LOGE(TAG, "Cannot receive message. End receiver process");
            break;
        }

        // Is it needed?
        // punch_hole_in_nat(sock);
    }
}

void udp_streamer_receiver_task(void *pvParameters)
{
    int port = (int)pvParameters;

    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    while (1)
    {
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            continue;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", port);

        process_receiver_connection(sock);

        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }

    vTaskDelete(NULL);
}
