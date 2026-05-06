#include "Storage.h"
#include "Globals.h"
#include "Audio.h"
#include <ArduinoJson.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>

#if !defined(ARDUINO)
#include <dirent.h>
#include <fstream>
#include <iostream>
#define SCORE_FILE "scores.dat"
#define SAVE_FILE "save.dat"
#else
#define SCORE_FILE "/scores.dat"
#define SAVE_FILE "/save.dat"
#endif

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

void saveGame() {
    FILE* f = fopen(SAVE_FILE, "wb");
    if (f) {
        fwrite(&playerProgress, sizeof(int), 1, f);
        fwrite(&playerLives, sizeof(int), 1, f);
        fwrite(&playerCoins, sizeof(int), 1, f);
        fwrite(&playerInventory, sizeof(Inventory), 1, f);
        fclose(f);
    }
}

void loadGame() {
    FILE* f = fopen(SAVE_FILE, "rb");
    if (f) {
        fread(&playerProgress, sizeof(int), 1, f);
        fread(&playerLives, sizeof(int), 1, f);
        fread(&playerCoins, sizeof(int), 1, f);
        if (fread(&playerInventory, sizeof(Inventory), 1, f) != 1) {
            playerInventory = { false, false, 0 };
        }
        fclose(f);
    } else {
        playerProgress = 1;
        playerLives = 5;
        playerCoins = 0;
        playerInventory = { false, false, 0 };
    }
}

void findMusic() {
    availableMusic.clear();
#if defined(ARDUINO)
    File root = LittleFS.open("/audio");
    if (!root || !root.isDirectory()) return;
    File file = root.openNextFile();
    while(file){
        std::string fn = file.name();
        if (fn.find_last_of("/") != std::string::npos) fn = fn.substr(fn.find_last_of("/") + 1);
        if (fn.size() > 4 && fn.substr(fn.size()-4) == ".mp3") {
            availableMusic.push_back(fn);
        }
        file = root.openNextFile();
    }
#else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("audio")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fn = ent->d_name;
            if (fn.size() > 4 && fn.substr(fn.size()-4) == ".mp3") {
                availableMusic.push_back(fn);
            }
        }
        closedir(dir);
    }
#endif
    std::sort(availableMusic.begin(), availableMusic.end());
}

void findSprites() {
    availableSprites.clear();
#if defined(ARDUINO)
    File root = LittleFS.open("/images");
    if (!root || !root.isDirectory()) return;
    File file = root.openNextFile();
    while(file){
        std::string fn = file.name();
        if (fn.find_last_of("/") != std::string::npos) fn = fn.substr(fn.find_last_of("/") + 1);
        if (fn.size() > 4 && fn.substr(fn.size()-4) == ".png") {
            availableSprites.push_back(fn);
        }
        file = root.openNextFile();
    }
#else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("images")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fn = ent->d_name;
            if (fn.size() > 4 && fn.substr(fn.size()-4) == ".png") {
                availableSprites.push_back(fn);
            }
        }
        closedir(dir);
    }
#endif
    std::sort(availableSprites.begin(), availableSprites.end());
}

void loadSpriteByName(const std::string& name) {
    cubeSprite.fillSprite(TFT_CYAN); // fallback
#if defined(ARDUINO)
    std::string path = "/images/" + name;
    if (LittleFS.exists(path.c_str())) {
        File f = LittleFS.open(path.c_str(), "r");
        if (f) {
            size_t sz = f.size();
            uint8_t* buf = (uint8_t*)malloc(sz);
            if (buf) {
                f.read(buf, sz);
                cubeSprite.drawPng(buf, sz, 0, 0, CUBE_SIZE, CUBE_SIZE);
                free(buf);
            }
            f.close();
        }
    }
#else
    std::string path = "images/" + name;
    FILE* f_check = fopen(path.c_str(), "r");
    if (f_check) {
        fclose(f_check);
        cubeSprite.drawPngFile(path.c_str(), 0, 0, CUBE_SIZE, CUBE_SIZE);
    }
#endif
}

void findLevels() {
    availableLevels.clear();
#if defined(ARDUINO)
    File root = LittleFS.open("/levels");
    if (!root || !root.isDirectory()) return;
    File file = root.openNextFile();
    while(file){
        std::string filename = file.name();
        if (filename.find_last_of("/") != std::string::npos) filename = filename.substr(filename.find_last_of("/") + 1);
        if (filename.find("level") != std::string::npos && filename.find(".json") != std::string::npos) {
            availableLevels.push_back(filename);
        }
        file = root.openNextFile();
    }
#else
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("levels")) != NULL) {
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
    snprintf(buf, sizeof(buf), "level%d.json", maxNum + 1);
    return std::string(buf);
}

void saveDesignerLevel(const char* filename) {
    JsonDocument doc;
    doc["finishX"] = 2000;
    doc["bgP"] = currentLevel.bgPattern;
    doc["bgM"] = currentLevel.bgMusic;
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
    std::string path = "/levels/" + std::string(filename);
    File f = LittleFS.open(path.c_str(), "w");
    f.print(jsonStr.c_str());
    f.close();
#else
    std::string path = "levels/" + std::string(filename);
    std::ofstream f(path);
    f << jsonStr;
    f.close();
#endif
    findLevels();
}

bool loadLevelByName(const char* filename) {
    char path[512];
    if (strstr(filename, "/")) strncpy(path, filename, sizeof(path));
    else snprintf(path, sizeof(path), "levels/%s", filename);

#if defined(ARDUINO)
    std::string devicePath = path;
    if (devicePath[0] != '/') devicePath = "/" + devicePath;
    File f = LittleFS.open(devicePath.c_str(), "r");
    if (!f) return false;
    std::string buffer = f.readString().c_str();
    f.close();
    const char* jsonPtr = buffer.c_str();
#else
    FILE* f = fopen(path, "r");
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
    currentLevel.bgPattern = doc["bgP"] | 0;
    currentLevel.bgMusic = doc["bgM"] | "bgm.mp3";
    
    // Play the music
    playAudio(currentLevel.bgMusic.c_str());
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
        else if (lo.type == OBST_SPIKE) { lo.w = 15; lo.h = 15; }
        else if (lo.type == OBST_PAD) { lo.w = 20; lo.h = 5; }
        else if (lo.type == OBST_COIN) { lo.w = 10; lo.h = 10; }
        else if (lo.type == OBST_PORTAL) { lo.w = 20; lo.h = 40; }
        currentLevel.objects.push_back(lo);
    }
    return true;
}

bool loadLevel(int levelNum) {
    char filename[32];
#if defined(ARDUINO)
    snprintf(filename, sizeof(filename), "level%d.json", levelNum);
#else
    snprintf(filename, sizeof(filename), "level%d.json", levelNum);
#endif
    return loadLevelByName(filename);
}

#include <map>
static std::map<std::string, uint16_t*> iconCache;

void loadIcon(const char* filename) {
    if (!filename) return;
    std::string fname(filename);
    if (iconCache.count(fname)) {
        iconSprite.pushImage(0, 0, 20, 20, iconCache[fname]);
        return;
    }

    iconSprite.fillSprite(0); // Transparent or black
#if defined(ARDUINO)
    std::string path = "/images/";
    path += filename;
    if (LittleFS.exists(path.c_str())) {
        File f = LittleFS.open(path.c_str(), "r");
        if (f) {
            size_t sz = f.size();
            uint8_t* buf = (uint8_t*)malloc(sz);
            if (buf) { f.read(buf, sz); iconSprite.drawPng(buf, sz, 0, 0, 20, 20); free(buf); }
            f.close();
        }
    }
#else
    std::string path = "images/" + std::string(filename);
    iconSprite.drawPngFile(path.c_str(), 0, 0, 20, 20);
#endif

    // Cache the loaded icon
    uint16_t* pixels = (uint16_t*)malloc(20 * 20 * 2);
    if (pixels) {
        for (int y = 0; y < 20; y++) {
            for (int x = 0; x < 20; x++) {
                pixels[y * 20 + x] = iconSprite.readPixel(x, y);
            }
        }
        iconCache[fname] = pixels;
    }
}
