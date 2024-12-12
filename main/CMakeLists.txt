idf_component_register(SRCS "udp_streamer_handler_receiver_message_processor.c" "main.c" "udps_handler.c" "wifi_handler.c" "web_handler.c" "udp_streamer_handler.c" "udp_streamer_handler_receiver.c" "udp_streamer_handler_controler.c" "udp_streamer_handler_receiver_message_processor.c"
                    PRIV_REQUIRES spi_flash nvs_flash esp_wifi esp_http_server esp_timer esp_driver_gpio
                    INCLUDE_DIRS "")
