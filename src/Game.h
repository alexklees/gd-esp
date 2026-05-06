#pragma once

#include "Config.h"
#include "Types.h"

void resetGame();
bool checkCollision();
void spawnBurst(float px, float py, int count, uint16_t color);
void updateInternal();
void update();
