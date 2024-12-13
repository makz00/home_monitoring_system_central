/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

#pragma once

#include "sys/time.h"

typedef int esp_err_t;

/**
 * @brief Data structure of UDP streamer frame buffer
 */
typedef struct
{
    size_t len;               /*!< Length of the buffer in bytes */
    size_t width;             /*!< Width of the buffer in pixels */
    size_t height;            /*!< Height of the buffer in pixels */
    struct timeval timestamp; /*!< Timestamp since boot of the first DMA buffer of the frame */
    uint8_t *buf;             /*!< Pointer to the pixel data */
} udps_fb_t;

/**
 * @brief UDP streamer configuration
 */
typedef struct
{
    const char *hostname;
    int control_port;
    int data_port;
    int buffered_fbs;
} udps_config_t;

/**
 * @brief Initialize the UDP streamer handler application protocol
 *
 * @param config  UDP streamer handler configuration parameters
 *
 * @return ESP_OK on success
 */
esp_err_t udp_streamer_handler_init(const udps_config_t *config);

/**
 * @brief Deinitialize the UDP streamer hadler
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_STATE if the application protocol hasn't been initialized yet
 */
esp_err_t udp_streamer_handler_deinit(void);

/**
 * @brief Obtain pointer to a frame buffer.
 *
 * @return pointer to the frame buffer
 */
udps_fb_t *udp_streamer_handler_fb_get(void);

/**
 * @brief Return the frame buffer to be reused again.
 *
 * @param fb    Pointer to the frame buffer
 */
void udp_streamer_handler_fb_return(udps_fb_t *fb);
