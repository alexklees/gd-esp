#pragma once

#include <vector>
#include <string>
#include <M5Unified.h>
#include <M5GFX.h>

#if defined(ARDUINO)
#include <Arduino.h>
#include <M5Cardputer.h>
#include <LittleFS.h>
#else
#define millis() lgfx::millis()
#endif



// Display constants
extern const int SCREEN_WIDTH;
extern const int SCREEN_HEIGHT;

// Game constants
extern const int FLOOR_Y; 
extern const int CUBE_SIZE;
extern const int CUBE_X;

extern const float GRAVITY;
extern const float JUMP_VELOCITY;

// Limits
#define MAX_PARTICLES 100
#define NUM_GHOSTS 5
#define MAX_BURST 50
#define MAX_SHOP_ITEMS 3
#define NUM_HIGH_SCORES 5
