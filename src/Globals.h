#pragma once

#include "Config.h"
#include "Types.h"
#include <M5GFX.h>

extern M5Canvas canvas;
extern M5Canvas cubeSprite;
extern M5Canvas iconSprite;

extern GameState state;

// Settings
extern int settingsSelectedRow;
extern int selectedSpriteIdx;
extern int difficultyLevel;
extern int volumeLevel;
extern const char* difficultyNames[];
extern const float difficultyBaseSpeed[];
extern std::vector<std::string> availableSprites;
extern std::vector<std::string> availableMusic;
extern std::vector<std::string> availableLevels;

// Shop
extern std::vector<PowerUpType> shopItems;
extern int selectedShopItem;
extern bool shopInitialized;
extern const char* powerUpNames[];
extern const int powerUpCosts[];
extern const char* powerUpSprites[];

// Persistent Player Data
extern int playerLives;
extern int playerCoins;
extern int playerProgress;
extern Inventory playerInventory;
extern HighScore highScores[NUM_HIGH_SCORES];

// Game Logic
extern LevelData currentLevel;
extern float cameraX;
extern int currentLevelIndex;
extern float obstacleSpeed;
extern int selectedLevelIdx;

// Designer
extern float designerCursorX;
extern float designerCursorY;
extern ObstacleType designerSelectedType;
extern char designerFilename[64];
extern bool isSavingDesigner;
extern bool isDesignerPath;
extern std::string designerSaveInput;
extern uint32_t designerMusicTimer;

// Player Physics
extern int score;
extern float cubeY;
extern float cubeVelocityY;
extern bool isJumping;
extern bool canDoubleJump;
extern float cubeAngle;
extern PlayerMode playerMode;

// High Score Input
extern int newHighScoreIndex;
extern char currentName[4];
extern int nameCharIndex;
extern uint32_t lastCharChangeTime;

// Visual Effects
extern Particle trails[MAX_PARTICLES];
extern int trailIdx;
extern int frameCount;

extern GhostFrame ghostTrail[NUM_GHOSTS];
extern int ghostIdx;

extern BurstParticle bursts[MAX_BURST];
extern int burstIdx;

extern uint32_t lastFrameTime;

#if defined(ARDUINO)
struct EspKeyState {
    bool enter, del, space;
    bool key_d, key_e, key_o, key_x, key_s;
    bool key_semicolon, key_period, key_comma, key_slash;
    bool key_left, key_right, ctrl, key_p, key_m, key_l;
    uint8_t numPressed;
    void capture() {
        auto& ks = M5Cardputer.Keyboard.keysState();
        enter = ks.enter;
        del   = ks.del;
        space = ks.space;
        key_d = M5Cardputer.Keyboard.isKeyPressed('d');
        key_e = M5Cardputer.Keyboard.isKeyPressed('e');
        key_o = M5Cardputer.Keyboard.isKeyPressed('o');
        key_x = M5Cardputer.Keyboard.isKeyPressed('x');
        key_s = M5Cardputer.Keyboard.isKeyPressed('s');
        key_semicolon = M5Cardputer.Keyboard.isKeyPressed(';');
        key_period    = M5Cardputer.Keyboard.isKeyPressed('.');
        key_comma     = M5Cardputer.Keyboard.isKeyPressed(',');
        key_slash     = M5Cardputer.Keyboard.isKeyPressed('/');
        key_left      = M5Cardputer.Keyboard.isKeyPressed(',');
        key_right     = M5Cardputer.Keyboard.isKeyPressed('/');
        key_p         = M5Cardputer.Keyboard.isKeyPressed('p');
        key_m         = M5Cardputer.Keyboard.isKeyPressed('m');
        key_l         = M5Cardputer.Keyboard.isKeyPressed('l');
        ctrl          = ks.ctrl;
        numPressed    = M5Cardputer.Keyboard.isPressed();
    }
    void clear() {
        enter = del = space = false;
        key_d = key_e = key_o = key_x = key_s = false;
        key_semicolon = key_period = key_comma = key_slash = false;
        key_left = key_right = ctrl = key_p = key_m = key_l = false;
        numPressed = 0;
    }
};
extern EspKeyState espCur, espPrev;
#endif
