/*
 * Home monitoring system
 * Author: Maksymilian Komarnicki
 */

#pragma once

#include "udp_streamer_handler_types.h"

void process_message(const udps_message_t *message);
