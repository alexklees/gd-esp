#if !defined(ARDUINO)
#include <SDL2/SDL.h>
#endif
#include <M5Unified.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <ArduinoJson.h>
#include <vector>

#if defined(ARDUINO)
#include <M5Cardputer.h>
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#define SCORE_FILE "/scores.dat"

AudioGeneratorMP3 *mp3_gen = nullptr;
AudioFileSourceLittleFS *audio_file = nullptr;
AudioOutputI2S *audio_out = nullptr;

void stopMP3() {
    if (mp3_gen) { mp3_gen->stop(); delete mp3_gen; mp3_gen = nullptr; }
    if (audio_file) { delete audio_file; audio_file = nullptr; }
}

void playMP3(const char* path) {
    stopMP3();
    audio_file = new AudioFileSourceLittleFS(path);
    mp3_gen = new AudioGeneratorMP3();
    mp3_gen->begin(audio_file, audio_out);
}
#else
#define SCORE_FILE "scores.dat"
#endif

// ── ESP32 keyboard edge-detection & debounce ──────────────────────────
#if defined(ARDUINO)
struct EspKeyState {
    bool enter, del, space;
    bool key_d, key_e, key_o, key_x;
    bool key_semicolon, key_period, key_comma, key_slash;
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
        key_semicolon = M5Cardputer.Keyboard.isKeyPressed(';');
        key_period    = M5Cardputer.Keyboard.isKeyPressed('.');
        key_comma     = M5Cardputer.Keyboard.isKeyPressed(',');
        key_slash     = M5Cardputer.Keyboard.isKeyPressed('/');
        numPressed    = M5Cardputer.Keyboard.isPressed();
    }
    void clear() {
        enter = del = space = false;
        key_d = key_e = key_o = key_x = false;
        key_semicolon = key_period = key_comma = key_slash = false;
        numPressed = 0;
    }
};
static EspKeyState espCur, espPrev;

// Rising-edge helpers: true only on the frame a key is first pressed
inline bool espEdge_enter()     { return espCur.enter && !espPrev.enter; }
inline bool espEdge_del()       { return espCur.del   && !espPrev.del; }
inline bool espEdge_space()     { return espCur.space && !espPrev.space; }
inline bool espEdge_d()         { return espCur.key_d && !espPrev.key_d; }
inline bool espEdge_e()         { return espCur.key_e && !espPrev.key_e; }
inline bool espEdge_o()         { return espCur.key_o && !espPrev.key_o; }
inline bool espEdge_x()         { return espCur.key_x && !espPrev.key_x; }
inline bool espEdge_up()        { return espCur.key_semicolon && !espPrev.key_semicolon; }
inline bool espEdge_down()      { return espCur.key_period    && !espPrev.key_period; }

// Debounce timer for menu navigation
static uint32_t lastMenuActionMs = 0;
const uint32_t  MENU_DEBOUNCE_MS = 120;
inline bool menuDebounceOk() {
    uint32_t now = millis();
    if (now - lastMenuActionMs < MENU_DEBOUNCE_MS) return false;
    lastMenuActionMs = now;
    return true;
}
#endif

#define FPS 60
#define FRAME_DELAY (1000 / FPS)

#if !defined(ARDUINO)
#define millis() lgfx::millis()
void stopAudio() {
    system("killall -9 afplay 2>/dev/null");
}

static const Uint8* currentKeyState = nullptr;
static Uint8 prevKeyState[SDL_NUM_SCANCODES] = {0};

bool isKeyPressed(SDL_Scancode key) {
    if (!currentKeyState) return false;
    return currentKeyState[key] && !prevKeyState[key];
}

void updateKeyboard() {
    currentKeyState = SDL_GetKeyboardState(NULL);
}
void postUpdateKeyboard() {
    if (currentKeyState) {
        memcpy(prevKeyState, currentKeyState, SDL_NUM_SCANCODES);
    }
}
#endif

M5Canvas canvas(&M5.Display);
M5Canvas cubeSprite(&canvas);

// Display size for Cardputer/StampS3 is typically 240x135
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 135;

// Game constants
const int FLOOR_Y = SCREEN_HEIGHT; 
const int CUBE_SIZE = 20;
const int CUBE_X = 40;

const float GRAVITY = 0.5f;
const float JUMP_VELOCITY = -7.5f;

// Game state
enum GameState { WELCOME, PLAYING, GAMEOVER, ENTER_NAME, LEVEL_COMPLETE, LEVEL_SELECT, DESIGNER };
GameState state = WELCOME;

struct HighScore {
    char name[4];
    int score;
};
const int NUM_HIGH_SCORES = 5;
HighScore highScores[NUM_HIGH_SCORES];

void loadHighScores() {
    FILE* f = fopen(SCORE_FILE, "rb");
    if (f) {
        fread(highScores, sizeof(HighScore), NUM_HIGH_SCORES, f);
        fclose(f);
    } else {
        for (int i = 0; i < NUM_HIGH_SCORES; i++) {
            strcpy(highScores[i].name, "---");
            highScores[i].score = 0;
        }
    }
}

void saveHighScores() {
    FILE* f = fopen(SCORE_FILE, "wb");
    if (f) {
        fwrite(highScores, sizeof(HighScore), NUM_HIGH_SCORES, f);
        fclose(f);
    }
}

int newHighScoreIndex = -1;
char currentName[4] = "AAA";
int nameCharIndex = 0;
uint32_t lastCharChangeTime = 0;
int score = 0;
float cubeY = FLOOR_Y - CUBE_SIZE;
float cubeVelocityY = 0;
bool isJumping = false;
float cubeAngle = 0.0f;

struct Particle {
    float x, y, angle;
    int life;
    float size;
};
const int MAX_PARTICLES = 60;
Particle trails[MAX_PARTICLES];
int trailIdx = 0;
int frameCount = 0;

enum ObstacleType { OBST_SPIKE = 1, OBST_BLOCK = 2, OBST_PAD = 3, OBST_COIN = 4 };
struct LevelObject {
    float x, y;
    int w, h;
    ObstacleType type;
    bool active;
};

struct LevelData {
    std::vector<LevelObject> objects;
    float finishX;
    int levelNum;
};

LevelData currentLevel;
float cameraX = 0;
int currentLevelIndex = 1;
float obstacleSpeed = 3.5f;

std::vector<std::string> availableLevels;
int selectedLevelIdx = 0;
float designerCursorX = 100, designerCursorY = 100;
ObstacleType designerSelectedType = OBST_BLOCK;
char designerFilename[64] = "";
bool isSavingDesigner = false;
bool isDesignerPath = false;
std::string designerSaveInput = "";

#if !defined(ARDUINO)
#include <dirent.h>
#include <fstream>
#include <iostream>
#endif

void findLevels() {
    availableLevels.clear();
#if defined(ARDUINO)
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file){
        std::string filename = file.name();
        if (filename[0] == '/') filename = filename.substr(1);
        if (filename.find("level") != std::string::npos && filename.find(".json") != std::string::npos) {
            availableLevels.push_back(filename);
        }
        file = root.openNextFile();
    }
#else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (".")) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        std::string filename = ent->d_name;
        if (filename.find("level") != std::string::npos && filename.find(".json") != std::string::npos) {
            availableLevels.push_back(filename);
        }
      }
      closedir (dir);
    }
#endif
    std::sort(availableLevels.begin(), availableLevels.end());
}

std::string getNextLevelFilename() {
    int maxNum = 0;
    for (const auto& name : availableLevels) {
        int n;
        if (sscanf(name.c_str(), "level%d.json", &n) == 1) {
            if (n > maxNum) maxNum = n;
        }
    }
    char buf[32];
    sprintf(buf, "level%d.json", maxNum + 1);
    return std::string(buf);
}

void saveDesignerLevel(const char* filename) {
    JsonDocument doc;
    doc["finishX"] = 2000;
    float maxObjX = 500;
    for (const auto& obj : currentLevel.objects) if (obj.x > maxObjX) maxObjX = obj.x;
    doc["finishX"] = maxObjX + 500;

    JsonArray objects = doc["objects"].to<JsonArray>();
    for (const auto& obj : currentLevel.objects) {
        JsonObject o = objects.add<JsonObject>();
        o["t"] = (int)obj.type;
        o["x"] = obj.x;
        o["y"] = obj.y;
    }
    
    std::string jsonStr;
    serializeJson(doc, jsonStr);
    
#if defined(ARDUINO)
    std::string path = "/";
    if (filename[0] != '/') path += filename;
    else path = filename;
    File f = LittleFS.open(path.c_str(), "w");
    f.print(jsonStr.c_str());
    f.close();
#else
    std::ofstream f(filename);
    f << jsonStr;
    f.close();
#endif
    findLevels();
}

uint32_t lastFrameTime = 0;

bool loadLevelByName(const char* filename) {
#if defined(ARDUINO)
    File f = LittleFS.open(filename, "r");
    if (!f) return false;
    std::string buffer = f.readString().c_str();
    f.close();
    const char* jsonPtr = buffer.c_str();
#else
    FILE* f = fopen(filename, "r");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);
    const char* jsonPtr = buffer;
#endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonPtr);
#if !defined(ARDUINO)
    free(buffer);
#endif

    if (error) return false;

    currentLevel.objects.clear();
    currentLevel.finishX = doc["finishX"];
    strncpy(designerFilename, filename, sizeof(designerFilename));
    
    // Extract level number from filename if possible
    int num = 1;
    sscanf(filename, "level%d.json", &num);
    currentLevel.levelNum = num;

    JsonArray objects = doc["objects"];
    for (JsonObject obj : objects) {
        LevelObject lo;
        lo.type = (ObstacleType)obj["t"];
        lo.x = obj["x"];
        lo.y = obj["y"];
        lo.active = true;
        if (lo.type == OBST_BLOCK) { lo.w = 20; lo.h = 20; }
        else if (lo.type == OBST_SPIKE) { lo.w = 16; lo.h = 20; }
        else if (lo.type == OBST_PAD) { lo.w = 30; lo.h = 5; }
        else if (lo.type == OBST_COIN) { lo.w = 10; lo.h = 10; }
        currentLevel.objects.push_back(lo);
    }
    return true;
}

bool loadLevel(int levelNum) {
    char filename[32];
#if defined(ARDUINO)
    snprintf(filename, sizeof(filename), "/level%d.json", levelNum);
#else
    snprintf(filename, sizeof(filename), "level%d.json", levelNum);
#endif
    return loadLevelByName(filename);
}

void resetGame() {
    cameraX = 0;
    if (!loadLevel(currentLevelIndex)) {
        currentLevelIndex = 1;
        loadLevel(1);
    }
    
    cubeY = FLOOR_Y - CUBE_SIZE;
    cubeVelocityY = 0;
    isJumping = false;
    cubeAngle = 0.0f;
    score = 0;
    obstacleSpeed = 3.5f + (currentLevelIndex * 0.0f);
    state = PLAYING;
    for (int i = 0; i < MAX_PARTICLES; i++) trails[i].life = 0;
#if defined(ARDUINO)
    playMP3("/bgm.mp3");
#else
    stopAudio();
    system("afplay bgm.mp3 &");
#endif
}

void setup() {
#if defined(ARDUINO)
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
#else
    auto cfg = M5.config();
    M5.begin(cfg);
#endif
    
#if defined(ARDUINO)
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
    }
    // Stop M5Unified's speaker so ESP8266Audio can take over the I2S bus
    M5.Speaker.end();
    // Cardputer/CardputerADV I2S: BCLK=41, WS=43, DATA_OUT=42, Port=I2S_NUM_1
    audio_out = new AudioOutputI2S(1);  // I2S port 1
    audio_out->SetPinout(41, 43, 42);
#endif

    loadHighScores();

#if defined(ARDUINO)
    playMP3("/bgm.mp3");
#else
    // Kill any existing instances and play the MP3 in the background
    stopAudio();
    system("afplay bgm.mp3 &");
#endif

    int width = M5.Display.width();
    int height = M5.Display.height();
    printf("Display size: %d x %d\n", width, height);
    
    // Increase speaker volume
    M5.Speaker.setVolume(128);
    
    // Create sprite for double buffering
    canvas.createSprite(width, height);
    
    // Create and setup cube sprite for rotation
    cubeSprite.createSprite(CUBE_SIZE, CUBE_SIZE);
    cubeSprite.fillSprite(TFT_CYAN); // Fallback color
    
#if defined(ARDUINO)
    // Attempt to load cat.png into the sprite using a buffer
    if (LittleFS.exists("/cat.png")) {
        File f = LittleFS.open("/cat.png", "r");
        if (f) {
            size_t size = f.size();
            uint8_t* buf = (uint8_t*)malloc(size);
            if (buf) {
                f.read(buf, size);
                cubeSprite.drawPng(buf, size, 0, 0, CUBE_SIZE, CUBE_SIZE);
                free(buf);
            }
            f.close();
        }
    }
#else
    // Attempt to load cat.png into the sprite (0, 0)
    cubeSprite.drawPngFile("cat.png", 0, 0, CUBE_SIZE, CUBE_SIZE);
#endif

    cubeSprite.setPivot(CUBE_SIZE / 2.0f, CUBE_SIZE / 2.0f);
}

bool checkCollision() {
    for (const auto& obj : currentLevel.objects) {
        if (!obj.active) continue;
        float screenX = obj.x - cameraX;
        if (screenX + obj.w < 0 || screenX > SCREEN_WIDTH) continue;

        if (obj.type == OBST_SPIKE) {
            if (CUBE_X + 2 < screenX + obj.w && CUBE_X + CUBE_SIZE - 2 > screenX &&
                cubeY + 2 < obj.y && cubeY + CUBE_SIZE - 2 > obj.y - obj.h) {
                return true;
            }
        } else if (obj.type == OBST_BLOCK) {
            if (CUBE_X + CUBE_SIZE - 2 > screenX && CUBE_X + 2 < screenX + obj.w) {
                if (cubeY + CUBE_SIZE - 2 > obj.y - obj.h + 5 && cubeY + 2 < obj.y) {
                    return true;
                }
            }
        }
    }
    return false;
}

void updateInternal() {
#if defined(ARDUINO)
    M5Cardputer.update();
    // Capture current keyboard state for edge detection
    espCur.capture();
    if (mp3_gen && mp3_gen->isRunning()) {
        if (!mp3_gen->loop()) stopMP3();
    }
#else
    M5.update();
#endif
    
    if (state == WELCOME) {
#if !defined(ARDUINO)
        if (isKeyPressed(SDL_SCANCODE_D)) {
            findLevels();
            isDesignerPath = true;
            selectedLevelIdx = 0;
            state = LEVEL_SELECT;
        }
        if (isKeyPressed(SDL_SCANCODE_SPACE)) {
            findLevels();
            isDesignerPath = false;
            selectedLevelIdx = 0;
            state = LEVEL_SELECT;
        }
#else
        if (espEdge_d()) {
            findLevels();
            isDesignerPath = true;
            selectedLevelIdx = 0;
            state = LEVEL_SELECT;
        }
        if (espEdge_enter() || espEdge_space()) {
            findLevels();
            isDesignerPath = false;
            selectedLevelIdx = 0;
            state = LEVEL_SELECT;
            return;
        }
#endif
        return;
    }

    if (state == LEVEL_SELECT) {
#if !defined(ARDUINO)
        if (isKeyPressed(SDL_SCANCODE_UP)) selectedLevelIdx = (selectedLevelIdx - 1 + (int)availableLevels.size() + 1) % (availableLevels.size() + 1);
        if (isKeyPressed(SDL_SCANCODE_DOWN)) selectedLevelIdx = (selectedLevelIdx + 1) % (availableLevels.size() + 1);
        if (isKeyPressed(SDL_SCANCODE_SPACE)) {
            if (selectedLevelIdx < availableLevels.size()) {
                // Extract level number from filename and set currentLevelIndex
                int num = 1;
                sscanf(availableLevels[selectedLevelIdx].c_str(), "level%d.json", &num);
                currentLevelIndex = num;
            }
            else { currentLevel.objects.clear(); currentLevel.finishX = 2000; designerFilename[0] = '\0'; }
            if (isDesignerPath) { loadLevel(currentLevelIndex); cameraX = 0; state = DESIGNER; }
            else { resetGame(); }
        }
        if (isKeyPressed(SDL_SCANCODE_ESCAPE)) state = WELCOME;
#else
        if (espEdge_up() && menuDebounceOk())
            selectedLevelIdx = (selectedLevelIdx - 1 + (int)availableLevels.size() + 1) % (availableLevels.size() + 1);
        if (espEdge_down() && menuDebounceOk())
            selectedLevelIdx = (selectedLevelIdx + 1) % (availableLevels.size() + 1);
        if ((espEdge_enter() || espEdge_space()) && menuDebounceOk()) {
            if (selectedLevelIdx < availableLevels.size()) { 
                std::string path = availableLevels[selectedLevelIdx];
                // Extract level number from filename and set currentLevelIndex
                int num = 1;
                sscanf(path.c_str(), "level%d.json", &num);
                if (path[0] == '/') sscanf(path.c_str(), "/level%d.json", &num);
                currentLevelIndex = num;
            }
            else { currentLevel.objects.clear(); currentLevel.finishX = 2000; designerFilename[0] = '\0'; }
            if (isDesignerPath) { loadLevel(currentLevelIndex); cameraX = 0; state = DESIGNER; }
            else { resetGame(); state = PLAYING; }
            return;
        }
#endif
        return;
    }

    if (state == DESIGNER) {
        bool left = false, right = false, up = false, down = false, enter = false, backspace = false, x_key = false, o_key = false, e_key = false, esc_key = false;
#if !defined(ARDUINO)
        const Uint8* k = SDL_GetKeyboardState(NULL);
        left = k[SDL_SCANCODE_LEFT]; right = k[SDL_SCANCODE_RIGHT]; up = k[SDL_SCANCODE_UP]; down = k[SDL_SCANCODE_DOWN];
        enter = isKeyPressed(SDL_SCANCODE_SPACE); backspace = isKeyPressed(SDL_SCANCODE_BACKSPACE);
        x_key = isKeyPressed(SDL_SCANCODE_X); o_key = isKeyPressed(SDL_SCANCODE_O); e_key = isKeyPressed(SDL_SCANCODE_E);
        esc_key = isKeyPressed(SDL_SCANCODE_ESCAPE);
#else
        // Continuous hold for cursor movement
        up    = espCur.key_semicolon;
        down  = espCur.key_period;
        left  = espCur.key_comma;
        right = espCur.key_slash;
        // Edge-detected for discrete actions
        o_key     = espEdge_o();
        e_key     = espEdge_e();
        x_key     = espEdge_x();
        enter     = espEdge_enter();
        backspace = espEdge_del();
#endif
        if (esc_key) { isSavingDesigner = false; state = WELCOME; return; }
        if (isSavingDesigner) {
#if !defined(ARDUINO)
            for (int i = SDL_SCANCODE_A; i <= SDL_SCANCODE_0; i++) {
                if (isKeyPressed((SDL_Scancode)i)) {
                    if (i <= SDL_SCANCODE_Z) designerSaveInput += (char)('a' + (i - SDL_SCANCODE_A));
                    else if (i >= SDL_SCANCODE_1 && i <= SDL_SCANCODE_9) designerSaveInput += (char)('1' + (i - SDL_SCANCODE_1));
                    else if (i == SDL_SCANCODE_0) designerSaveInput += '0';
                }
            }
            if (isKeyPressed(SDL_SCANCODE_PERIOD)) designerSaveInput += ".";
#else
            // Only accept new character input on rising edge (key count increased)
            if (espCur.numPressed > 0 && espCur.numPressed > espPrev.numPressed) {
                auto& status = M5Cardputer.Keyboard.keysState();
                for (auto i : status.word) {
                    char c = (char)i;
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.') {
                        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
                        designerSaveInput += c;
                    }
                }
            }
            if (espEdge_enter()) enter = true;
            if (espEdge_del()) backspace = true;
#endif
            if (backspace && designerSaveInput.length() > 0) designerSaveInput.pop_back();
            if (enter) { saveDesignerLevel(designerSaveInput.c_str()); isSavingDesigner = false; state = WELCOME; }
        } else {
            if (left) designerCursorX -= 5; if (right) designerCursorX += 5; if (up) designerCursorY -= 5; if (down) designerCursorY += 5;
            if (designerCursorX < cameraX) cameraX = designerCursorX;
            if (designerCursorX > cameraX + SCREEN_WIDTH) cameraX = designerCursorX - SCREEN_WIDTH;
            if (o_key) { designerSelectedType = (ObstacleType)((int)designerSelectedType + 1); if (designerSelectedType > OBST_COIN) designerSelectedType = OBST_SPIKE; }
            if (enter) {
                LevelObject lo; lo.x = designerCursorX; lo.y = designerCursorY; lo.type = designerSelectedType; lo.active = true;
                if (lo.type == OBST_BLOCK) { lo.w = 20; lo.h = 20; } else if (lo.type == OBST_SPIKE) { lo.w = 16; lo.h = 20; } else if (lo.type == OBST_PAD) { lo.w = 30; lo.h = 5; } else if (lo.type == OBST_COIN) { lo.w = 10; lo.h = 10; }
                currentLevel.objects.push_back(lo);
            }
            if (e_key) {
                for (auto it = currentLevel.objects.begin(); it != currentLevel.objects.end(); ) {
                    if (designerCursorX >= it->x && designerCursorX <= it->x + it->w && designerCursorY >= it->y - it->h && designerCursorY <= it->y) { it = currentLevel.objects.erase(it); } else { ++it; }
                }
            }
            if (x_key) { isSavingDesigner = true; if (strlen(designerFilename) > 0) designerSaveInput = designerFilename; else designerSaveInput = getNextLevelFilename(); }
        }
        return;
    }

    if (state == PLAYING) {
#if !defined(ARDUINO)
        if (isKeyPressed(SDL_SCANCODE_ESCAPE)) { state = WELCOME; stopAudio(); return; }
#else
        if (espEdge_del()) { state = WELCOME; stopMP3(); return; }
#endif
    }
    
    if (state == ENTER_NAME) {
#if !defined(ARDUINO)
        for (int i = SDL_SCANCODE_A; i <= SDL_SCANCODE_Z; i++) { if (isKeyPressed((SDL_Scancode)i)) { if (nameCharIndex < 3) { currentName[nameCharIndex] = 'A' + (i - SDL_SCANCODE_A); nameCharIndex++; } } }
        if (isKeyPressed(SDL_SCANCODE_BACKSPACE)) { if (nameCharIndex > 0) { nameCharIndex--; currentName[nameCharIndex] = ' '; } }
        if (isKeyPressed(SDL_SCANCODE_SPACE)) { if (nameCharIndex == 3) { strcpy(highScores[newHighScoreIndex].name, currentName); saveHighScores(); state = WELCOME; } }
#else
        uint32_t currentMs = millis();
        if (espEdge_enter() || espEdge_space()) {
            if (currentName[nameCharIndex] == ' ') currentName[nameCharIndex] = 'A'; else currentName[nameCharIndex]++;
            if (currentName[nameCharIndex] > 'Z') currentName[nameCharIndex] = 'A';
            lastCharChangeTime = currentMs;
        }
        if (currentMs - lastCharChangeTime > 1500 && currentName[nameCharIndex] != ' ') {
            nameCharIndex++;
            if (nameCharIndex >= 3) { strcpy(highScores[newHighScoreIndex].name, currentName); saveHighScores(); state = WELCOME; } else { lastCharChangeTime = currentMs; }
        }
#endif
        return;
    }

    if (state == LEVEL_COMPLETE) {
#if !defined(ARDUINO)
        bool pressed = isKeyPressed(SDL_SCANCODE_SPACE);
        if (pressed) {
#else
        if (espEdge_enter() || espEdge_space()) {
#endif
            currentLevelIndex++;
            if (currentLevelIndex > 10) currentLevelIndex = 1;
            resetGame();
            state = PLAYING;
            return;
        }
        return;
    }

    bool jumpPressed = false;
#if !defined(ARDUINO)
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_SPACE]) jumpPressed = true;
#else
    if (espCur.space) jumpPressed = true;
#endif
    
    if (state == GAMEOVER) {
#if !defined(ARDUINO)
        bool pressed = isKeyPressed(SDL_SCANCODE_SPACE);
        if (pressed) {
#else
        if (espEdge_enter() || espEdge_space()) {
#endif
            state = WELCOME;
            return;
        }
        return;
    }

    // Update level progress
    cameraX += obstacleSpeed;
    if (cameraX > currentLevel.finishX) {
        state = LEVEL_COMPLETE;
        return;
    }

    if (jumpPressed && !isJumping) {
        cubeVelocityY = JUMP_VELOCITY;
        isJumping = true;
        M5.Speaker.tone(800, 50, 1); // Jump sound on channel 1
    }

    // Apply gravity
    cubeVelocityY += GRAVITY;
    cubeY += cubeVelocityY;
    frameCount++;

    if (isJumping) {
        cubeAngle += 4.0f;
        
        // Spawn particle trail every 3 frames for sparseness
        if (frameCount % 3 == 0) {
            float randX = (rand() % 5) - 2.5f;
            float randY = (rand() % 5) - 2.5f;
            float randSize = 0.5f + (rand() % 100) / 200.0f;
            trails[trailIdx] = { CUBE_X + randX, cubeY + randY, cubeAngle, 30, randSize };
            trailIdx = (trailIdx + 1) % MAX_PARTICLES;
        }
    }
    
    // Update particle trails
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (trails[i].life > 0) {
            trails[i].x -= obstacleSpeed;
            trails[i].life--;
        }
    }

    // Floor collision
    float groundLevel = FLOOR_Y;
    
    // Platform landing logic
    for (const auto& obj : currentLevel.objects) {
        if (obj.type == OBST_BLOCK) {
            float screenX = obj.x - cameraX;
            if (CUBE_X + CUBE_SIZE > screenX + 2 && CUBE_X < screenX + obj.w - 2) {
                if (cubeY + CUBE_SIZE <= obj.y - obj.h + 10 && cubeY + CUBE_SIZE >= obj.y - obj.h - 5) {
                    groundLevel = obj.y - obj.h;
                }
            }
        }
    }

    if (cubeY >= groundLevel - CUBE_SIZE) {
        cubeY = groundLevel - CUBE_SIZE;
        cubeVelocityY = 0;
        isJumping = false;
        
        // Snap angle to nearest 90 degrees
        int angleInt = (int)(cubeAngle + 45);
        cubeAngle = (angleInt / 90) * 90;
    } else {
        isJumping = true; // If we fall off a block
    }
    
    // Pad auto-jump logic
    for (const auto& obj : currentLevel.objects) {
        if (obj.type == OBST_PAD) {
            float screenX = obj.x - cameraX;
            bool withinX = (CUBE_X + CUBE_SIZE - 2 > screenX && CUBE_X + 2 < screenX + obj.w);
            bool touchingPad = (cubeY + CUBE_SIZE >= obj.y - obj.h);
            if (withinX && touchingPad && cubeVelocityY >= 0) {
                cubeVelocityY = JUMP_VELOCITY;
                isJumping = true;
                M5.Speaker.tone(800, 50, 1);
            }
        }
    }

    // Coin collection logic
    for (auto& obj : currentLevel.objects) {
        if (obj.active && obj.type == OBST_COIN) {
            float screenX = obj.x - cameraX;
            if (CUBE_X + CUBE_SIZE > screenX && CUBE_X < screenX + obj.w &&
                cubeY + CUBE_SIZE > obj.y - obj.h && cubeY < obj.y) {
                obj.active = false;
                score++;
                M5.Speaker.tone(1200, 50);
            }
        }
    }

    // Check overlaps
    if (checkCollision()) {
        M5.Speaker.tone(200, 200, 1); // Die sound on channel 1
#if !defined(ARDUINO)
        stopAudio();
#endif
        
        newHighScoreIndex = -1;
        // In level mode, score could be levels completed or distance. 
        // For now, let's just use currentLevelIndex as score or just ignore high scores for levels.
        state = GAMEOVER;
    }
}

void update() {
#if !defined(ARDUINO)
    updateKeyboard();
#endif
    updateInternal();
#if !defined(ARDUINO)
    postUpdateKeyboard();
#else
    // Save current state as previous for next frame's edge detection
    espPrev = espCur;
#endif
}

void draw() {
    canvas.clear(TFT_BLACK);

    int screenWidth = M5.Display.width();

    if (state == WELCOME) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_CYAN);
        canvas.drawString("Snoopy Dash", screenWidth / 2, SCREEN_HEIGHT / 4 - 10);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
        if ((millis() / 500) % 2 == 0) {
#if !defined(ARDUINO)
            canvas.drawString("Press Space to Start", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
#else
            canvas.drawString("Press OK to Start", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
#endif
        }
#if !defined(ARDUINO)
        canvas.drawString("Press 'D' for Designer", screenWidth / 2, SCREEN_HEIGHT / 4 + 30);
#endif
        
        int scrollX = screenWidth - ((millis() / 20) % (screenWidth + 300));
        char hsText[128] = "HIGH SCORES: ";
        for (int i=0; i<NUM_HIGH_SCORES; i++) {
            char temp[16];
            sprintf(temp, "%d.%s-%d ", i+1, highScores[i].name, highScores[i].score);
            strcat(hsText, temp);
        }
        canvas.setTextDatum(TL_DATUM);
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString(hsText, scrollX, SCREEN_HEIGHT - 20);
        
        canvas.pushSprite(0, 0);
        return;
    }
    
    if (state == LEVEL_SELECT) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString("SELECT LEVEL", screenWidth / 2, 20);
        
        canvas.setTextSize(1);
        for (int i=0; i<availableLevels.size(); i++) {
            uint16_t color = (i == selectedLevelIdx) ? TFT_GREEN : TFT_WHITE;
            canvas.setTextColor(color);
            canvas.drawString(availableLevels[i].c_str(), screenWidth / 2, 50 + i * 15);
        }
        uint16_t color = (selectedLevelIdx == availableLevels.size()) ? TFT_GREEN : TFT_YELLOW;
        canvas.setTextColor(color);
        canvas.drawString("< CREATE NEW LEVEL >", screenWidth / 2, 50 + availableLevels.size() * 15);
        
        canvas.pushSprite(0, 0);
        return;
    }

    if (state == DESIGNER) {
        canvas.clear(TFT_DARKGREY);
        // Draw grid
        for (int x = 0; x < SCREEN_WIDTH; x += 20) canvas.drawLine(x, 0, x, SCREEN_HEIGHT, TFT_BLACK);
        for (int y = 0; y < SCREEN_HEIGHT; y += 20) canvas.drawLine(0, y, SCREEN_WIDTH, y, TFT_BLACK);

        // Draw current objects
        for (const auto& obj : currentLevel.objects) {
            float screenX = obj.x - cameraX;
            if (obj.type == OBST_SPIKE) canvas.fillTriangle(screenX, obj.y, screenX + obj.w/2, obj.y - obj.h, screenX + obj.w, obj.y, TFT_RED);
            else if (obj.type == OBST_BLOCK) canvas.fillRect(screenX, obj.y - obj.h, obj.w, obj.h, TFT_ORANGE);
            else if (obj.type == OBST_PAD) canvas.fillRect(screenX, obj.y - obj.h, obj.w, obj.h, TFT_YELLOW);
            else if (obj.type == OBST_COIN) canvas.fillCircle(screenX + obj.w/2, obj.y - obj.h/2, obj.w/2, TFT_BLUE);
        }

        // Draw cursor object
        float curScreenX = designerCursorX - cameraX;
        canvas.drawRect(curScreenX, designerCursorY - 10, 10, 10, TFT_WHITE);
        
        canvas.setTextDatum(TL_DATUM);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        canvas.printf("X:%.0f Y:%.0f | O:Cycle Space:Place E:Del X:Save", designerCursorX, designerCursorY);
#else
        canvas.printf("X:%.0f Y:%.0f | O:Cycle Ent:Place E:Del X:Save", designerCursorX, designerCursorY);
#endif
        
        if (isSavingDesigner) {
            canvas.fillRect(20, SCREEN_HEIGHT / 2 - 20, SCREEN_WIDTH - 40, 40, TFT_BLACK);
            canvas.drawRect(20, SCREEN_HEIGHT / 2 - 20, SCREEN_WIDTH - 40, 40, TFT_WHITE);
            canvas.setTextDatum(MC_DATUM);
            canvas.drawString("Save as:", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);
            canvas.drawString(designerSaveInput.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 5);
        }

        canvas.pushSprite(0, 0);
        return;
    }
    
    if (state == ENTER_NAME) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString("NEW HIGH SCORE!", screenWidth / 2, SCREEN_HEIGHT / 4 - 10);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        canvas.drawString("Type name, Space to save.", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
#else
        canvas.drawString("Press OK to cycle letter.", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
        canvas.drawString("Wait 1.5s to confirm.", screenWidth / 2, SCREEN_HEIGHT / 4 + 30);
#endif
        
        canvas.setTextSize(3);
        int startX = screenWidth / 2 - 30;
        for (int i=0; i<3; i++) {
            uint16_t color = (i == nameCharIndex) ? TFT_RED : TFT_WHITE;
            canvas.setTextColor(color);
            char c = currentName[i];
            if (c == ' ' || c == '\0') c = '_';
            canvas.drawChar(c, startX + (i * 30), SCREEN_HEIGHT / 2 + 10);
        }
        
        canvas.setTextDatum(TL_DATUM);
        canvas.pushSprite(0, 0);
        return;
    }

    if (state == LEVEL_COMPLETE) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_GREEN);
        canvas.drawString("LEVEL COMPLETE!", screenWidth / 2, SCREEN_HEIGHT / 2 - 20);
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        canvas.drawString("Press Space for next level", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
#else
        canvas.drawString("Press OK for next level", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
#endif
        canvas.pushSprite(0, 0);
        return;
    }

    // Draw floor
    canvas.drawLine(0, FLOOR_Y, screenWidth, FLOOR_Y, TFT_WHITE);

    // Draw obstacles
    for (const auto& obj : currentLevel.objects) {
        float screenX = obj.x - cameraX;
        if (screenX + obj.w < 0 || screenX > SCREEN_WIDTH) continue;

        if (obj.type == OBST_SPIKE) {
            int x1 = (int)screenX;
            int y1 = (int)obj.y;
            int x2 = (int)screenX + (obj.w / 2);
            int y2 = (int)obj.y - obj.h;
            int x3 = (int)screenX + obj.w;
            int y3 = (int)obj.y;
            canvas.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_RED);
        } else if (obj.type == OBST_BLOCK) {
            canvas.fillRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, TFT_ORANGE);
            canvas.drawRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, TFT_WHITE);
        } else if (obj.type == OBST_PAD) {
            canvas.fillRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, TFT_YELLOW);
        } else if (obj.type == OBST_COIN) {
            if (obj.active) {
                canvas.fillCircle((int)screenX + obj.w / 2, (int)obj.y - obj.h / 2, obj.w / 2, TFT_BLUE);
                canvas.drawCircle((int)screenX + obj.w / 2, (int)obj.y - obj.h / 2, obj.w / 2 + 1, TFT_CYAN);
            }
        }
    }

    // Draw finish line
    float finishScreenX = currentLevel.finishX - cameraX;
    if (finishScreenX >= 0 && finishScreenX < SCREEN_WIDTH) {
        for (int i = 0; i < SCREEN_HEIGHT; i += 10) {
            canvas.fillRect((int)finishScreenX, i, 10, 10, (i / 10 % 2 == 0) ? TFT_WHITE : TFT_BLACK);
        }
    }

    // Draw particle trails
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (trails[i].life > 0) {
            float scale = (trails[i].life / 30.0f) * trails[i].size;
            cubeSprite.pushRotateZoom(&canvas, trails[i].x + CUBE_SIZE / 2.0f, trails[i].y + CUBE_SIZE / 2.0f, trails[i].angle, scale, scale);
        }
    }

    // Draw player
    cubeSprite.pushRotateZoom(&canvas, CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE / 2.0f, cubeAngle, 1.0f, 1.0f);

    // Draw Level progress
    canvas.setTextSize(1);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(5, 5);
    int progress = (int)(cameraX * 100 / currentLevel.finishX);
    if (progress > 100) progress = 100;
    canvas.printf("Level %d: %d%% | Coins: %d", currentLevelIndex, progress, score);

    // Draw Game Over overlay
    if (state == GAMEOVER) {
        canvas.setTextDatum(MC_DATUM); // Middle center
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_RED);
        canvas.drawString("GAME OVER", screenWidth / 2, SCREEN_HEIGHT / 2 - 10);
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        canvas.drawString("Press Space to Restart", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
#else
        canvas.drawString("Press OK to Restart", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
#endif
        canvas.setTextDatum(TL_DATUM); // Reset to Top Left
    }

    // Push buffer to display
    canvas.pushSprite(0, 0);
}

void loop() {
    uint32_t currentMillis = millis();
    if (currentMillis - lastFrameTime >= FRAME_DELAY) {
        lastFrameTime = currentMillis;
        
        update();
        draw();
    }
}

#if !defined(ARDUINO)

int user_thread(bool* running) {
    setup();
    printf("Hello World!\n");
    while (*running) {
        loop();
        lgfx::delay(1);
    }
    return 0;
}

void handle_signal(int sig) {
    stopAudio();
    exit(sig);
}

int main(int argc, char **argv) {
    atexit(stopAudio);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    int result = lgfx::Panel_sdl::main(user_thread);
    stopAudio();
    return result;
}
#endif
