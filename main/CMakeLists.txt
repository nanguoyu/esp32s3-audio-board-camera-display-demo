idf_component_register(SRCS "main.c" "board.c" "audio.c" "prism_ui.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_spi esp_driver_ledc
    esp_lcd esp_timer esp_driver_i2s esp_driver_usb_serial_jtag vfs espressif__esp32-camera espressif__esp_jpeg
    espressif__led_strip espressif__esp_codec_dev)
