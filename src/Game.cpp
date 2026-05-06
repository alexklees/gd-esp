#include "Game.h"
#include "Globals.h"
#include "Audio.h"
#include "Storage.h"
#include "Shop.h"
#include "Input.h"
#include "Render.h"
#include <math.h>

void spawnBurst(float px, float py, int count, uint16_t color) {
    for (int i = 0; i < count; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 1.0f + (rand() % 30) / 10.0f;
        bursts[burstIdx] = { px, py, cosf(angle) * speed, sinf(angle) * speed, 20 + (rand() % 10), color };
        burstIdx = (burstIdx + 1) % MAX_BURST;
    }
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
    canDoubleJump = playerInventory.hasDoubleJump;
    cubeAngle = 0.0f;
    playerMode = MODE_CUBE;
    score = 0;
    obstacleSpeed = difficultyBaseSpeed[difficultyLevel];
    state = PLAYING;
    for (int i = 0; i < MAX_PARTICLES; i++) trails[i].life = 0;
    playAudio(currentLevel.bgMusic.c_str());
}

bool checkCollision() {
    if (playerMode == MODE_SHIP) {
        if (cubeY < 0 || cubeY > FLOOR_Y - CUBE_SIZE) return true;
    }

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
                    if (playerInventory.shieldHits > 0) {
                        playerInventory.shieldHits--;
                        spawnBurst(CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE / 2.0f, 15, TFT_GREEN);
                        M5.Speaker.tone(1500, 100);
                        return false;
                    }
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
        if (isKeyPressed(SDL_SCANCODE_S)) {
            findSprites();
            settingsSelectedRow = 0;
            state = SETTINGS;
        }
        if (isKeyPressed(SDL_SCANCODE_SPACE)) {
            resetGame();
            state = PLAYING;
        }
        if (isKeyPressed(SDL_SCANCODE_L)) {
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
        if (espEdge_s() && menuDebounceOk()) {
            findSprites();
            settingsSelectedRow = 0;
            state = SETTINGS;
            return;
        }
        if (espEdge_l()) {
            findLevels();
            isDesignerPath = false;
            selectedLevelIdx = 0;
            state = LEVEL_SELECT;
            return;
        }
        if (espEdge_enter() || espEdge_space()) {
            resetGame();
            state = PLAYING;
            return;
        }
#endif
        return;
    }

    if (state == SETTINGS) {
#if !defined(ARDUINO)
        if (isKeyPressed(SDL_SCANCODE_UP)) settingsSelectedRow = (settingsSelectedRow - 1 + 3) % 3;
        if (isKeyPressed(SDL_SCANCODE_DOWN)) settingsSelectedRow = (settingsSelectedRow + 1) % 3;
        if (isKeyPressed(SDL_SCANCODE_LEFT) || isKeyPressed(SDL_SCANCODE_RIGHT)) {
            int dir = isKeyPressed(SDL_SCANCODE_RIGHT) ? 1 : -1;
            if (settingsSelectedRow == 0 && !availableSprites.empty()) {
                selectedSpriteIdx = (selectedSpriteIdx + dir + (int)availableSprites.size()) % (int)availableSprites.size();
                loadSpriteByName(availableSprites[selectedSpriteIdx]);
            } else if (settingsSelectedRow == 1) {
                difficultyLevel = std::max(0, std::min(3, difficultyLevel + dir));
            } else if (settingsSelectedRow == 2) {
                volumeLevel = std::max(0, std::min(10, volumeLevel + dir));
                applyVolume();
            }
        }
        if (isKeyPressed(SDL_SCANCODE_ESCAPE) || isKeyPressed(SDL_SCANCODE_SPACE)) state = WELCOME;
#else
        if (espEdge_up() && menuDebounceOk()) settingsSelectedRow = (settingsSelectedRow - 1 + 3) % 3;
        if (espEdge_down() && menuDebounceOk()) settingsSelectedRow = (settingsSelectedRow + 1) % 3;
        bool left = espEdge_left() && menuDebounceOk();
        bool right = espEdge_right() && menuDebounceOk();
        if (left || right) {
            int dir = right ? 1 : -1;
            if (settingsSelectedRow == 0 && !availableSprites.empty()) {
                selectedSpriteIdx = (selectedSpriteIdx + dir + (int)availableSprites.size()) % (int)availableSprites.size();
                loadSpriteByName(availableSprites[selectedSpriteIdx]);
            } else if (settingsSelectedRow == 1) {
                difficultyLevel = std::max(0, std::min(3, difficultyLevel + dir));
            } else if (settingsSelectedRow == 2) {
                volumeLevel = std::max(0, std::min(10, volumeLevel + dir));
                applyVolume();
            }
        }
        if ((espEdge_enter() || espEdge_del()) && menuDebounceOk()) { state = WELCOME; return; }
#endif
        return;
    }

    if (state == LEVEL_SELECT) {
#if !defined(ARDUINO)
        if (isKeyPressed(SDL_SCANCODE_UP)) selectedLevelIdx = (selectedLevelIdx - 1 + (int)availableLevels.size() + 1) % (availableLevels.size() + 1);
        if (isKeyPressed(SDL_SCANCODE_DOWN)) selectedLevelIdx = (selectedLevelIdx + 1) % (availableLevels.size() + 1);
        if (isKeyPressed(SDL_SCANCODE_SPACE)) {
            if (selectedLevelIdx < availableLevels.size()) {
                int num = 1;
                sscanf(availableLevels[selectedLevelIdx].c_str(), "level%d.json", &num);
                currentLevelIndex = num;
            } else { currentLevel.objects.clear(); currentLevel.finishX = 2000; designerFilename[0] = '\0'; }
            
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
                int num = 1;
                sscanf(path.c_str(), "level%d.json", &num);
                if (path[0] == '/') sscanf(path.c_str(), "/level%d.json", &num);
                currentLevelIndex = num;
            } else { currentLevel.objects.clear(); currentLevel.finishX = 2000; designerFilename[0] = '\0'; }
            
            if (isDesignerPath) { loadLevel(currentLevelIndex); cameraX = 0; state = DESIGNER; }
            else { resetGame(); state = PLAYING; }
            return;
        }
#endif
        return;
    }

    if (state == DESIGNER) {
        bool left = false, right = false, up = false, down = false, enter = false, backspace = false, x_key = false, o_key = false, e_key = false, esc_key = false, ctrl = false, p_key = false, m_key = false;
#if !defined(ARDUINO)
        const Uint8* k = SDL_GetKeyboardState(NULL);
        left = k[SDL_SCANCODE_LEFT]; right = k[SDL_SCANCODE_RIGHT]; up = k[SDL_SCANCODE_UP]; down = k[SDL_SCANCODE_DOWN];
        enter = isKeyPressed(SDL_SCANCODE_SPACE); backspace = isKeyPressed(SDL_SCANCODE_BACKSPACE);
        x_key = isKeyPressed(SDL_SCANCODE_X); o_key = isKeyPressed(SDL_SCANCODE_O); e_key = isKeyPressed(SDL_SCANCODE_E);
        esc_key = isKeyPressed(SDL_SCANCODE_ESCAPE);
        ctrl = k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL] || k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT];
        p_key = isKeyPressed(SDL_SCANCODE_P);
        m_key = isKeyPressed(SDL_SCANCODE_M);
#else
        up    = espCur.key_semicolon;
        down  = espCur.key_period;
        left  = espCur.key_comma;
        right = espCur.key_slash;
        o_key     = espEdge_o();
        e_key     = espEdge_e();
        x_key     = espEdge_x();
        enter     = espEdge_enter();
        backspace = espEdge_del();
        ctrl      = espCur.ctrl;
        p_key     = espEdge_p();
        m_key     = espEdge_m();
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
            float moveAmt = ctrl ? 1.0f : 5.0f;
            if (left) designerCursorX -= moveAmt; if (right) designerCursorX += moveAmt; if (up) designerCursorY -= moveAmt; if (down) designerCursorY += moveAmt;
            if (p_key) { currentLevel.bgPattern = (currentLevel.bgPattern + 1) % 9; }
            if (m_key && !availableMusic.empty()) {
                int curIdx = 0;
                for (int i=0; i<availableMusic.size(); i++) if (availableMusic[i] == currentLevel.bgMusic) curIdx = i;
                currentLevel.bgMusic = availableMusic[(curIdx + 1) % availableMusic.size()];
                playAudio(currentLevel.bgMusic.c_str());
#if defined(ARDUINO)
                designerMusicTimer = millis();
#else
                designerMusicTimer = SDL_GetTicks();
#endif
            }
            if (designerCursorY < 0) designerCursorY = 0;
            if (designerCursorY > SCREEN_HEIGHT) designerCursorY = SCREEN_HEIGHT;
            if (designerCursorX < 0) designerCursorX = 0;
            if (designerCursorX < cameraX) cameraX = designerCursorX;
            if (designerCursorX > cameraX + SCREEN_WIDTH - 20) cameraX = designerCursorX - (SCREEN_WIDTH - 20);
            if (cameraX < 0) cameraX = 0;
            if (o_key) { designerSelectedType = (ObstacleType)((int)designerSelectedType + 1); if (designerSelectedType > OBST_PORTAL) designerSelectedType = OBST_SPIKE; }
            if (enter) {
                LevelObject lo; lo.x = designerCursorX; lo.y = designerCursorY; lo.type = designerSelectedType; lo.active = true;
                if (lo.type == OBST_BLOCK) { lo.w = 20; lo.h = 20; } 
                else if (lo.type == OBST_SPIKE) { lo.w = 16; lo.h = 20; } 
                else if (lo.type == OBST_PAD) { lo.w = 30; lo.h = 5; } 
                else if (lo.type == OBST_COIN) { lo.w = 10; lo.h = 10; }
                else if (lo.type == OBST_PORTAL) { lo.w = 20; lo.h = 40; }
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
        if (espEdge_del()) { state = WELCOME; stopAudio(); return; }
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
            playerProgress++;
            playerCoins += score;
            if (playerProgress > 10) playerProgress = 1;
            currentLevelIndex = playerProgress;
            saveGame();
            
            generateShopItems();
            state = SHOP;
            return;
        }
        return;
    }

    if (state == SHOP) {
        bool left = false, right = false, enter = false, del = false;
#if !defined(ARDUINO)
        left = isKeyPressed(SDL_SCANCODE_LEFT);
        right = isKeyPressed(SDL_SCANCODE_RIGHT);
        enter = isKeyPressed(SDL_SCANCODE_SPACE);
        del = isKeyPressed(SDL_SCANCODE_ESCAPE);
#else
        left = espEdge_left() && menuDebounceOk();
        right = espEdge_right() && menuDebounceOk();
        enter = (espEdge_enter() || espEdge_space()) && menuDebounceOk();
        del = espEdge_del() && menuDebounceOk();
#endif
        if (left) selectedShopItem = (selectedShopItem - 1 + (int)shopItems.size()) % (int)shopItems.size();
        if (right) selectedShopItem = (selectedShopItem + 1) % (int)shopItems.size();
        if (enter) {
            PowerUpType type = shopItems[selectedShopItem];
            int cost = powerUpCosts[type];
            if (playerCoins >= cost) {
                playerCoins -= cost;
                if (type == PU_DOUBLE_JUMP) playerInventory.hasDoubleJump = true;
                else if (type == PU_EXTRA_LIFE) playerLives++;
                else if (type == PU_BIG_JUMP) playerInventory.hasBigJump = true;
                else if (type == PU_SHIELD) playerInventory.shieldHits = 3;
                saveGame();
                M5.Speaker.tone(1000, 100);
            } else {
                M5.Speaker.tone(200, 100);
            }
        }
        if (del) {
            resetGame();
            state = PLAYING;
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
            playerProgress = 1;
            playerLives = 5;
            playerCoins = 0;
            playerInventory = { false, false, 0 };
            currentLevelIndex = 1;
            saveGame();
            state = WELCOME;
            return;
        }
        return;
    }

    if (state == DEATH) {
#if !defined(ARDUINO)
        bool pressed = isKeyPressed(SDL_SCANCODE_SPACE);
        if (pressed) {
#else
        if (espEdge_enter() || espEdge_space()) {
#endif
            resetGame();
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

    if (playerMode == MODE_CUBE) {
        if (jumpPressed) {
            if (!isJumping) {
                float baseJump = JUMP_VELOCITY;
                if (playerInventory.hasBigJump) baseJump *= 1.3f;
                cubeVelocityY = baseJump;
                isJumping = true;
                canDoubleJump = playerInventory.hasDoubleJump;
                M5.Speaker.tone(800, 50, 1);
                spawnBurst(CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE, 8, hslToRgb565(colorHue, 0.9f, 0.6f));
            } else if (canDoubleJump) {
                bool edge = false;
#if !defined(ARDUINO)
                edge = isKeyPressed(SDL_SCANCODE_SPACE);
#else
                edge = espEdge_space();
#endif
                if (edge) {
                    float baseJump = JUMP_VELOCITY;
                    if (playerInventory.hasBigJump) baseJump *= 1.3f;
                    cubeVelocityY = baseJump;
                    canDoubleJump = false;
                    M5.Speaker.tone(1000, 50, 1);
                    spawnBurst(CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE / 2.0f, 12, TFT_CYAN);
                }
            }
        }
        cubeVelocityY += GRAVITY;
    } else {
        // SHIP MODE: Continuous movement
        float thrust = jumpPressed ? -3.0f : 3.0f;
        cubeVelocityY = thrust;
        // Angle follows velocity
        cubeAngle = cubeVelocityY * 10.0f; 
        isJumping = true; // Always "jumping" in ship mode
    }

    cubeY += cubeVelocityY;
    frameCount++;

    // Update ghost trail every 2 frames
    if (frameCount % 2 == 0) {
        ghostTrail[ghostIdx] = { (float)CUBE_X, cubeY, cubeAngle };
        ghostIdx = (ghostIdx + 1) % NUM_GHOSTS;
    }

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

    // Update burst particles
    for (int i = 0; i < MAX_BURST; i++) {
        if (bursts[i].life > 0) {
            bursts[i].x += bursts[i].vx - obstacleSpeed;
            bursts[i].y += bursts[i].vy;
            bursts[i].vy += 0.15f; // mini gravity
            bursts[i].life--;
        }
    }

    cycleColors();

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
        canDoubleJump = playerInventory.hasDoubleJump;
        
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
                float baseJump = JUMP_VELOCITY;
                if (playerInventory.hasBigJump) baseJump *= 1.3f;
                cubeVelocityY = baseJump;
                isJumping = true;
                canDoubleJump = playerInventory.hasDoubleJump;
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

    // Portal logic
    for (auto& obj : currentLevel.objects) {
        if (obj.active && obj.type == OBST_PORTAL) {
            float screenX = obj.x - cameraX;
            if (CUBE_X + CUBE_SIZE > screenX && CUBE_X < screenX + obj.w &&
                cubeY + CUBE_SIZE > obj.y - obj.h && cubeY < obj.y) {
                playerMode = (playerMode == MODE_CUBE) ? MODE_SHIP : MODE_CUBE;
                cubeVelocityY = 0;
                obj.active = false;
                spawnBurst(CUBE_X + CUBE_SIZE/2.0f, cubeY + CUBE_SIZE/2.0f, 15, playerMode == MODE_SHIP ? TFT_MAGENTA : TFT_CYAN);
                M5.Speaker.tone(playerMode == MODE_SHIP ? 1200 : 800, 100);
            }
        }
    }

    // Check overlaps
    if (checkCollision()) {
        if (playerInventory.shieldHits > 0) {
            playerInventory.shieldHits--;
            spawnBurst(CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE / 2.0f, 15, TFT_GREEN);
            M5.Speaker.tone(1500, 100);
        } else {
            newHighScoreIndex = -1;
            M5.Speaker.tone(200, 200, 1); // Die sound on channel 1
            stopAudio();
            spawnBurst(CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE / 2.0f, 20, TFT_RED);
            playerLives--;
            saveGame();
            if (playerLives > 0) {
                state = DEATH;
            } else {
                state = GAMEOVER;
            }
        }
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
    espPrev = espCur;
#endif
}
