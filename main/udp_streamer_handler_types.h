/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

#pragma once

#include <sys/time.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include <freertos/task.h>

#include "esp_bit_defs.h"

#define MESSAGE_BUFFER_SIZE 1400

#define MSG_ASS_OWNED_BIT 0x02
#define MSG_ASS_USAGE_BIT 0x01

#define MSG_ASS_PRODUCER_OWNED_VAL (0 << 1)
#define MSG_ASS_CONSUMER_OWNED_VAL (1 << 1)
#define MSG_ASS_FREE_VAL (0 << 0)
#define MSG_ASS_USED_VAL (1 << 0)

typedef struct
{
    size_t len;
    size_t width;
    size_t height;
    struct timeval timestamp;
    int msg_total;
    int msg_number;
    int msg_len;
    uint8_t buf[MESSAGE_BUFFER_SIZE];
} udps_message_t;

typedef struct
{
    size_t len;
    size_t width;
    size_t height;
    struct timeval timestamp;
    int msg_total;
    int msg_received;
    uint8_t bits;
    uint8_t *buf;
} udps_message_assembly_t;

typedef struct
{
    // Buffers part of UDPS state
    udps_message_assembly_t *fbs_messages_buf;
    int fbs_messages_buf_size;
    udps_message_assembly_t *fbs_message_oldest;

    // Task part of UDPS state
    TaskHandle_t receiver_task_handle;
    TaskHandle_t control_task_handle;


} udps_state_t;
