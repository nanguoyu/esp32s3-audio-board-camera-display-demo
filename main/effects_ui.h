#pragma once
#include <stdbool.h>
#include <stdint.h>

#define EFFECT_COUNT 4
extern const char *const effect_names[EFFECT_COUNT];
extern const uint8_t effect_colors[EFFECT_COUNT][3];
void effects_render(uint16_t *screen,const uint16_t *camera,int effect,bool camera_ok,
                  bool frozen,bool muted,int level,int fps,bool flash);
