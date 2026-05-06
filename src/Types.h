#pragma once

#include <vector>
#include <string>

enum GameState { WELCOME, PLAYING, GAMEOVER, ENTER_NAME, LEVEL_COMPLETE, LEVEL_SELECT, DESIGNER, SETTINGS, SHOP, DEATH };

enum ObstacleType { OBST_SPIKE = 1, OBST_BLOCK = 2, OBST_PAD = 3, OBST_COIN = 4, OBST_PORTAL = 5 };
enum PlayerMode { MODE_CUBE, MODE_SHIP };

struct LevelObject {
    ObstacleType type;
    float x;
    float y;
    int w = 20;
    int h = 20;
    bool active = true; // For coins
};

struct LevelData {
    std::vector<LevelObject> objects;
    float finishX;
    int levelNum;
    int bgPattern; // 0=Mountains, 1=Squares, 2=Hexagons, 3=Stars
    std::string bgMusic;
};

enum PowerUpType { PU_DOUBLE_JUMP, PU_EXTRA_LIFE, PU_BIG_JUMP, PU_SHIELD, PU_COUNT };

struct Inventory {
    bool hasDoubleJump;
    bool hasBigJump;
    int shieldHits;
};

struct HighScore {
    char name[4];
    int score;
};

struct Particle {
    float x, y, angle;
    int life;
    float size;
};

struct BurstParticle {
    float x, y, vx, vy;
    int life;
    uint16_t color;
};

struct GhostFrame {
    float x, y, angle;
};
