#pragma once

#include "Config.h"
#include "Types.h"
#include <string>

void loadHighScores();
void saveHighScores();

void saveGame();
void loadGame();

void findMusic();
void findSprites();
void findLevels();

std::string getNextLevelFilename();
void saveDesignerLevel(const char* filename);
bool loadLevelByName(const char* filename);
bool loadLevel(int levelNum);

void loadSpriteByName(const std::string& name);
void loadIcon(const char* filename);
