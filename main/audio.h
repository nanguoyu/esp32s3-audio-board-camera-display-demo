#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t app_audio_init(void);
void app_audio_sound(int kind, int variation);
void app_audio_mute(bool muted);
uint16_t app_audio_level(void);
uint32_t app_audio_blocks(void);
