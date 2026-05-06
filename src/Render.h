#pragma once

#include "Config.h"
#include "Types.h"
#include <stdint.h>

extern float colorHue;
extern bool bgInitialized;

// Background structures
struct Star { float x, y; uint8_t brightness; };
extern Star bgStars[];

struct Mountain { float x; int h; int w; };
extern Mountain bgMountains[];

void initBackground();
uint16_t hslToRgb565(float h, float s, float l);
void cycleColors();

void drawGradientSky();
void drawParallaxBackground();
void drawShop();
void draw();
