#include "Config.h"
#include "Types.h"
#include "Globals.h"
#include "Audio.h"
#include "Storage.h"
#include "Input.h"
#include "Shop.h"
#include "Game.h"
#include "Render.h"

#if !defined(ARDUINO)
#include <SDL2/SDL.h>
#define millis() lgfx::millis()
#endif

void setup() {
#if defined(ARDUINO)
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
    }
    M5.Speaker.end();
    audio_out = new AudioOutputI2S(1);
    audio_out->SetPinout(41, 43, 42);
#else
    auto cfg = M5.config();
    M5.begin(cfg);
#endif

    srand(millis());

    loadHighScores();
    loadGame();
    currentLevelIndex = playerProgress;
    findSprites();
    findMusic();

    playAudio("bgm.mp3");

    int width = M5.Display.width();
    int height = M5.Display.height();
    printf("Display size: %d x %d\n", width, height);
    
    applyVolume();
    
    canvas.createSprite(width, height);
    cubeSprite.createSprite(CUBE_SIZE, CUBE_SIZE);
    cubeSprite.fillSprite(TFT_CYAN);
    iconSprite.createSprite(20, 20);
    
#if defined(ARDUINO)
    if (LittleFS.exists("/images/cat.png")) {
        File f = LittleFS.open("/images/cat.png", "r");
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
    cubeSprite.drawPngFile("images/cat.png", 0, 0, CUBE_SIZE, CUBE_SIZE);
#endif

    cubeSprite.setPivot(CUBE_SIZE / 2.0f, CUBE_SIZE / 2.0f);
    iconSprite.setPivot(10, 10);
}

void loop() {
    uint32_t currentMillis = millis();
#if !defined(ARDUINO)
    #define FRAME_DELAY (1000 / 60)
#else
    #define FRAME_DELAY (1000 / 60)
#endif
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

int main(int argc, char** argv) {
    int result = lgfx::Panel_sdl::main(user_thread);
    stopAudio();
    return result;
}
#endif
