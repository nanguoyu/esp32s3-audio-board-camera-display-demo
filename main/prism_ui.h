#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PRISM_EFFECTS 4
extern const char *const prism_names[PRISM_EFFECTS];
extern const uint8_t prism_colors[PRISM_EFFECTS][3];
void prism_render(uint16_t *screen,const uint16_t *camera,int effect,bool camera_ok,
                  bool frozen,bool muted,int level,int fps,bool flash);
