#include "Globals.h"

M5Canvas canvas(&M5.Display);
M5Canvas cubeSprite(&canvas);
M5Canvas iconSprite(&canvas);

GameState state = WELCOME;

// Settings
int settingsSelectedRow = 0;
int selectedSpriteIdx = 0;
int difficultyLevel = 1;
int volumeLevel = 5;
const char* difficultyNames[] = { "Easy", "Normal", "Hard", "Extreme" };
const float difficultyBaseSpeed[] = { 2.5f, 3.5f, 5.0f, 7.0f };
std::vector<std::string> availableSprites;
std::vector<std::string> availableMusic;
std::vector<std::string> availableLevels;

// Shop
std::vector<PowerUpType> shopItems;
int selectedShopItem = 0;
bool shopInitialized = false;
const char* powerUpNames[] = { "Double Jump", "Extra Life", "Big Jump", "Shield" };
const int powerUpCosts[] = { 50, 25, 40, 30 };
const char* powerUpSprites[] = { "double_jump.png", "extra_life.png", "big_jump.png", "shield.png" };

// Persistent Player Data
int playerLives = 5;
int playerCoins = 0;
int playerProgress = 1;
Inventory playerInventory = { false, false, 0 };
HighScore highScores[NUM_HIGH_SCORES];

// Game Logic
LevelData currentLevel;
float cameraX = 0;
int currentLevelIndex = 1;
float obstacleSpeed = 3.5f;
int selectedLevelIdx = 0;

// Designer
float designerCursorX = 100;
float designerCursorY = 100;
ObstacleType designerSelectedType = OBST_BLOCK;
char designerFilename[64] = "";
bool isSavingDesigner = false;
bool isDesignerPath = false;
std::string designerSaveInput = "";
uint32_t designerMusicTimer = 0;

// Player Physics
int score = 0;
float cubeY = FLOOR_Y - CUBE_SIZE;
float cubeVelocityY = 0;
bool isJumping = false;
bool canDoubleJump = false;
float cubeAngle = 0.0f;
PlayerMode playerMode = MODE_CUBE;

// High Score Input
int newHighScoreIndex = -1;
char currentName[4] = "AAA";
int nameCharIndex = 0;
uint32_t lastCharChangeTime = 0;

// Visual Effects
Particle trails[MAX_PARTICLES];
int trailIdx = 0;
int frameCount = 0;

GhostFrame ghostTrail[NUM_GHOSTS];
int ghostIdx = 0;

BurstParticle bursts[MAX_BURST];
int burstIdx = 0;

uint32_t lastFrameTime = 0;

#if defined(ARDUINO)
EspKeyState espCur, espPrev;
#endif

// Define constants from Config.h here
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 135;
const int FLOOR_Y = SCREEN_HEIGHT; 
const int CUBE_SIZE = 20;
const int CUBE_X = 40;
const float GRAVITY = 0.5f;
const float JUMP_VELOCITY = -7.5f;
