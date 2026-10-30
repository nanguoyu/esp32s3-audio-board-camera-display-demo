#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "esp_camera.h"

#define SCREEN_W 240
#define SCREEN_H 280
#define CAMERA_W 240
#define CAMERA_H 240

extern i2c_master_bus_handle_t board_i2c;
esp_err_t board_init(void);
esp_err_t board_camera_init(void);
esp_err_t board_lcd_init(void);
esp_err_t board_lcd_gpio_test(void);
esp_err_t board_lcd_draw(const uint16_t *pixels);
esp_err_t board_touch_init(void);
bool board_touch_read(int *x, int *y);
esp_err_t board_expander_update(uint16_t mask, uint16_t value);
uint16_t board_buttons(void);
void board_ring(uint8_t r, uint8_t g, uint8_t b);
void board_scan(void);
