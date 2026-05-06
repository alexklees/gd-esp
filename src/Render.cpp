#include "Render.h"
#include "Globals.h"
#include "Storage.h"
#include <math.h>

float colorHue = 0.0f;

const int NUM_STARS_LOCAL = 30; // avoiding redefined multiple times
Star bgStars[NUM_STARS_LOCAL];

const int NUM_MOUNTAINS_LOCAL = 12;
Mountain bgMountains[NUM_MOUNTAINS_LOCAL];

bool bgInitialized = false;

void initBackground() {
    for (int i = 0; i < NUM_STARS_LOCAL; i++) {
        bgStars[i] = { (float)(rand() % SCREEN_WIDTH), (float)(rand() % (SCREEN_HEIGHT - 30)), (uint8_t)(80 + rand() % 176) };
    }
    for (int i = 0; i < NUM_MOUNTAINS_LOCAL; i++) {
        bgMountains[i] = { (float)(i * (SCREEN_WIDTH / 4) + rand() % 40), 20 + rand() % 35, 40 + rand() % 40 };
    }
    bgInitialized = true;
}

uint16_t hslToRgb565(float h, float s, float l) {
    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    float r = 0, g = 0, b = 0;
    if (h < 60)       { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else              { r = c; b = x; }
    uint8_t R = (uint8_t)((r + m) * 255);
    uint8_t G = (uint8_t)((g + m) * 255);
    uint8_t B = (uint8_t)((b + m) * 255);
    return ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
}

void cycleColors() {
    colorHue += 0.15f;
    if (colorHue >= 360.0f) colorHue -= 360.0f;
}

void drawGradientSky() {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float t = (float)y / SCREEN_HEIGHT;
        float hBase = 240.0f + colorHue * 0.1f;
        if (hBase >= 360.0f) hBase -= 360.0f;
        float h = hBase + t * 40.0f;
        if (h >= 360.0f) h -= 360.0f;
        float l = 0.08f + t * 0.06f;
        canvas.drawFastHLine(0, y, SCREEN_WIDTH, hslToRgb565(h, 0.6f, l));
    }
}

void drawParallaxBackground() {
    if (!bgInitialized) initBackground();

    float starOffset = cameraX * 0.1f;
    for (int i = 0; i < NUM_STARS_LOCAL; i++) {
        float sx = bgStars[i].x - fmodf(starOffset, (float)SCREEN_WIDTH);
        if (sx < 0) sx += SCREEN_WIDTH;
        if (sx >= SCREEN_WIDTH) sx -= SCREEN_WIDTH;
        uint8_t b = bgStars[i].brightness;
        float twinkle = 0.6f + 0.4f * sinf((float)(frameCount + i * 37) * 0.08f);
        b = (uint8_t)(b * twinkle);
        uint16_t col = ((b >> 3) << 11) | ((b >> 2) << 5) | (b >> 3);
        canvas.drawPixel((int)sx, (int)bgStars[i].y, col);
        if (bgStars[i].brightness > 180) {
            canvas.drawPixel((int)sx + 1, (int)bgStars[i].y, col);
        }
    }

    float mtOffset = fmodf(cameraX * 0.3f, (float)(SCREEN_WIDTH * 3));
    uint16_t mtColor = hslToRgb565(fmodf(colorHue + 180.0f, 360.0f), 0.3f, 0.12f);
    for (int i = 0; i < NUM_MOUNTAINS_LOCAL; i++) {
        float mx = bgMountains[i].x - fmodf(mtOffset, (float)(SCREEN_WIDTH + 100));
        if (mx < -80) mx += SCREEN_WIDTH + 100;
        int mh = bgMountains[i].h;
        int mw = bgMountains[i].w;
        int baseY = FLOOR_Y - 1;

        switch (currentLevel.bgPattern) {
            case 1:
                canvas.fillRect((int)mx, baseY - mh, mw, mh, mtColor);
                canvas.drawRect((int)mx, baseY - mh, mw, mh, canvas.color565(40, 40, 40));
                break;
            case 2:
                canvas.fillCircle((int)mx + mw/2, baseY - mh/2, mw/2, mtColor);
                break;
            case 3:
                canvas.fillTriangle((int)mx + mw/2, baseY - mh, (int)mx, baseY - mh/2, (int)mx + mw, baseY - mh/2, mtColor);
                canvas.fillTriangle((int)mx + mw/2, baseY, (int)mx, baseY - mh/2, (int)mx + mw, baseY - mh/2, mtColor);
                break;
            case 4:
                for (int ty = FLOOR_Y; ty > -mh; ty -= mh) {
                    canvas.drawTriangle((int)mx, ty, (int)mx + mw / 2, ty - mh, (int)mx + mw, ty, mtColor);
                    canvas.drawTriangle((int)mx + 5, ty - 2, (int)mx + mw / 2, ty - mh + 5, (int)mx + mw - 5, ty - 2, canvas.color565(40, 40, 80));
                }
                break;
            case 5:
                for (int ty = FLOOR_Y; ty > -mh; ty -= mh) {
                    canvas.drawRect((int)mx, ty - mh, mw, mh, mtColor);
                    canvas.drawRect((int)mx + 2, ty - mh + 2, mw - 4, mh - 4, canvas.color565(40, 40, 80));
                }
                break;
            case 6:
                {
                    int segs = 6;
                    int sh = SCREEN_HEIGHT / segs;
                    for (int s=0; s<segs; s++) {
                        int ox = (s % 2 == 0) ? 10 : -10;
                        canvas.fillRect((int)mx + ox, s * sh, mw, sh, mtColor);
                    }
                }
                break;
            case 7:
                {
                    uint16_t neon = (i % 3 == 0) ? TFT_CYAN : (i % 3 == 1) ? TFT_MAGENTA : TFT_GREEN;
                    for (int ty = -mh; ty < SCREEN_HEIGHT + mh; ty += mh) {
                        canvas.drawLine((int)mx, ty, (int)mx + mw, ty - mh, neon);
                    }
                }
                break;
            case 8:
                canvas.drawFastVLine((int)mx + mw/2, 0, SCREEN_HEIGHT, canvas.color565(40, 40, 60));
                for (int ty = 10; ty < SCREEN_HEIGHT; ty += 30) {
                    canvas.fillRect((int)mx + mw/2 - 2, ty, 4, 4, mtColor);
                    if ((i + ty) % 60 == 0) canvas.drawRect((int)mx + mw/2 - 6, ty - 4, 12, 12, TFT_WHITE);
                }
                break;
            default:
                canvas.fillTriangle((int)mx, baseY, (int)mx + mw / 2, baseY - mh, (int)mx + mw, baseY, mtColor);
                break;
        }
    }
}

void drawShop() {
    int screenWidth = SCREEN_WIDTH;
    drawGradientSky();
    
    canvas.setTextDatum(MC_DATUM);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_CYAN);
    canvas.drawString("POWER-UP SHOP", screenWidth / 2, 20);
    
    canvas.setTextSize(1);
    canvas.setTextColor(TFT_YELLOW);
    char coinsBuf[32];
    snprintf(coinsBuf, sizeof(coinsBuf), "Your Coins: %d", playerCoins);
    canvas.drawString(coinsBuf, screenWidth / 2, 40);

    for (int i = 0; i < (int)shopItems.size(); i++) {
        int x = 40 + i * 80;
        int y = 80;
        bool selected = (i == selectedShopItem);
        
        PowerUpType type = shopItems[i];
        
        uint16_t cardCol = selected ? hslToRgb565(colorHue, 0.8f, 0.2f) : canvas.color565(30, 30, 30);
        canvas.fillRoundRect(x - 35, y - 30, 70, 60, 5, cardCol);
        canvas.drawRoundRect(x - 35, y - 30, 70, 60, 5, selected ? TFT_GREEN : TFT_WHITE);
        
        loadIcon(powerUpSprites[type]);
        iconSprite.pushSprite(x - 10, y - 25);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString(powerUpNames[type], x, y + 5);
        
        canvas.setTextColor(TFT_YELLOW);
        char costBuf[16];
        snprintf(costBuf, sizeof(costBuf), "%d C", powerUpCosts[type]);
        canvas.drawString(costBuf, x, y + 18);
    }
    
    canvas.setTextDatum(BC_DATUM);
    canvas.setTextColor(TFT_DARKGREY);
#if !defined(ARDUINO)
    canvas.drawString("Arrows:Nav | Space:Buy | Esc:Level", screenWidth / 2, SCREEN_HEIGHT - 5);
#else
    canvas.drawString(",. Nav | OK:Buy | Del:Level", screenWidth / 2, SCREEN_HEIGHT - 5);
#endif
}

void draw() {
    drawGradientSky();

    int screenWidth = M5.Display.width();

    if (state == WELCOME) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_CYAN);
        canvas.drawString("Snoopy Dash", screenWidth / 2, SCREEN_HEIGHT / 4 - 10);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        if ((millis() / 500) % 2 == 0) {
            canvas.drawString("Press Space to Start", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
        }
        canvas.drawString("'D' Designer | 'S' Settings | 'L' Level Select", screenWidth / 2, SCREEN_HEIGHT / 4 + 30);
#else
        if ((millis() / 500) % 2 == 0) {
            canvas.drawString("Press OK to Start", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
        }
        canvas.setTextColor(TFT_DARKGREY);
        canvas.drawString("'S' Settings | 'D' Designer | 'L' Levels", screenWidth / 2, SCREEN_HEIGHT / 4 + 30);
#endif
        
        int scrollX = screenWidth - ((millis() / 20) % (screenWidth + 300));
        char hsText[128] = "HIGH SCORES: ";
        for (int i=0; i<NUM_HIGH_SCORES; i++) {
            char temp[16];
            snprintf(temp, sizeof(temp), "%d.%s-%d ", i+1, highScores[i].name, highScores[i].score);
            strcat(hsText, temp);
        }
        canvas.setTextDatum(TL_DATUM);
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString(hsText, scrollX, SCREEN_HEIGHT - 20);
        
        canvas.pushSprite(0, 0);
        return;
    }

    if (state == SETTINGS) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_CYAN);
        canvas.drawString("SETTINGS", screenWidth / 2, 15);
        
        canvas.setTextSize(1);
        const int rowY[] = { 45, 70, 95 };
        const char* rowLabels[] = { "Sprite", "Difficulty", "Volume" };
        
        for (int r = 0; r < 3; r++) {
            bool selected = (r == settingsSelectedRow);
            canvas.setTextColor(selected ? TFT_GREEN : TFT_WHITE);
            
            canvas.setTextDatum(ML_DATUM);
            canvas.drawString(rowLabels[r], 5, rowY[r]);
            
            canvas.setTextDatum(MR_DATUM);
            char valBuf[32];
            if (r == 0) {
                if (availableSprites.empty()) {
                    snprintf(valBuf, sizeof(valBuf), "< none >");
                } else {
                    snprintf(valBuf, sizeof(valBuf), "< %s >", availableSprites[selectedSpriteIdx].c_str());
                }
                canvas.drawString(valBuf, screenWidth - 5, rowY[r]);
            } else if (r == 1) {
                snprintf(valBuf, sizeof(valBuf), "< %s >", difficultyNames[difficultyLevel]);
                if (selected) {
                    uint16_t dColors[] = { TFT_GREEN, TFT_YELLOW, TFT_ORANGE, TFT_RED };
                    canvas.setTextColor(dColors[difficultyLevel]);
                }
                canvas.drawString(valBuf, screenWidth - 5, rowY[r]);
            } else {
                int barX = screenWidth - 90;
                int barW = 80;
                int barH = 8;
                int barY = rowY[r] - barH / 2;
                canvas.drawRect(barX, barY, barW, barH, selected ? TFT_GREEN : TFT_WHITE);
                int fillW = (barW - 2) * volumeLevel / 10;
                if (fillW > 0) {
                    canvas.fillRect(barX + 1, barY + 1, fillW, barH - 2, selected ? TFT_GREEN : TFT_WHITE);
                }
                snprintf(valBuf, sizeof(valBuf), "%d", volumeLevel);
                canvas.setTextDatum(MR_DATUM);
                canvas.drawString(valBuf, barX - 4, rowY[r]);
            }
        }
        
        cubeSprite.pushRotateZoom(&canvas, screenWidth / 2.0f, 120.0f, 0, 1.0f, 1.0f);
        
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(TFT_DARKGREY);
#if !defined(ARDUINO)
        canvas.drawString("Arrows:Nav | Space:Back", screenWidth / 2, SCREEN_HEIGHT - 5);
#else
        canvas.drawString(";. Nav  ,/ Adj  OK:Back", screenWidth / 2, SCREEN_HEIGHT - 5);
#endif
        
        canvas.pushSprite(0, 0);
        return;
    }
    
    if (state == LEVEL_SELECT) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString("SELECT LEVEL", screenWidth / 2, 18);
        
        canvas.setTextSize(1);
        
        const int listStartY = 40;
        const int rowHeight = 15;
        const int maxVisibleRows = (SCREEN_HEIGHT - listStartY - 5) / rowHeight;
        int totalItems = (int)availableLevels.size() + 1;
        
        static int scrollOffset = 0;
        if (selectedLevelIdx < scrollOffset) scrollOffset = selectedLevelIdx;
        if (selectedLevelIdx >= scrollOffset + maxVisibleRows) scrollOffset = selectedLevelIdx - maxVisibleRows + 1;
        if (scrollOffset < 0) scrollOffset = 0;
        
        for (int row = 0; row < maxVisibleRows && (scrollOffset + row) < totalItems; row++) {
            int itemIdx = scrollOffset + row;
            int drawY = listStartY + row * rowHeight;
            
            if (itemIdx < (int)availableLevels.size()) {
                uint16_t color = (itemIdx == selectedLevelIdx) ? TFT_GREEN : TFT_WHITE;
                canvas.setTextColor(color);
                if (itemIdx == selectedLevelIdx) {
                    std::string sel = "> " + availableLevels[itemIdx] + " <";
                    canvas.drawString(sel.c_str(), screenWidth / 2, drawY);
                } else {
                    canvas.drawString(availableLevels[itemIdx].c_str(), screenWidth / 2, drawY);
                }
            } else {
                uint16_t color = (selectedLevelIdx == (int)availableLevels.size()) ? TFT_GREEN : TFT_YELLOW;
                canvas.setTextColor(color);
                if (selectedLevelIdx == (int)availableLevels.size()) {
                    canvas.drawString("> CREATE NEW <", screenWidth / 2, drawY);
                } else {
                    canvas.drawString("< CREATE NEW >", screenWidth / 2, drawY);
                }
            }
        }
        
        canvas.setTextColor(TFT_DARKGREY);
        if (scrollOffset > 0) {
            canvas.drawString("^", screenWidth / 2, listStartY - 8);
        }
        if (scrollOffset + maxVisibleRows < totalItems) {
            canvas.drawString("v", screenWidth / 2, SCREEN_HEIGHT - 5);
        }
        
        canvas.pushSprite(0, 0);
        return;
    }

    if (state == DESIGNER) {
        drawGradientSky();
        drawParallaxBackground();
        for (int x = 0; x < SCREEN_WIDTH; x += 20) canvas.drawLine(x, 0, x, SCREEN_HEIGHT, canvas.color565(20, 20, 20));
        for (int y = 0; y < SCREEN_HEIGHT; y += 20) canvas.drawLine(0, y, SCREEN_WIDTH, y, TFT_BLACK);

        for (const auto& obj : currentLevel.objects) {
            float screenX = obj.x - cameraX;
            if (obj.type == OBST_SPIKE) canvas.fillTriangle(screenX, obj.y, screenX + obj.w/2, obj.y - obj.h, screenX + obj.w, obj.y, TFT_RED);
            else if (obj.type == OBST_BLOCK) canvas.fillRect(screenX, obj.y - obj.h, obj.w, obj.h, TFT_ORANGE);
            else if (obj.type == OBST_PAD) canvas.fillRect(screenX, obj.y - obj.h, obj.w, obj.h, TFT_YELLOW);
            else if (obj.type == OBST_COIN) canvas.fillCircle(screenX + obj.w/2, obj.y - obj.h/2, obj.w/2, TFT_BLUE);
            else if (obj.type == OBST_PORTAL) {
                canvas.fillRoundRect(screenX, obj.y - obj.h, obj.w, obj.h, 5, TFT_MAGENTA);
                canvas.drawRoundRect(screenX, obj.y - obj.h, obj.w, obj.h, 5, TFT_WHITE);
            }
        }

        float curScreenX = designerCursorX - cameraX;
        int cw = 20, ch = 20; 
        if (designerSelectedType == OBST_BLOCK) { cw = 20; ch = 20; canvas.fillRect(curScreenX, designerCursorY - ch, cw, ch, TFT_ORANGE); }
        else if (designerSelectedType == OBST_SPIKE) { cw = 16; ch = 20; canvas.fillTriangle(curScreenX, designerCursorY, curScreenX + cw/2, designerCursorY - ch, curScreenX + cw, designerCursorY, TFT_RED); }
        else if (designerSelectedType == OBST_PAD) { cw = 30; ch = 5; canvas.fillRect(curScreenX, designerCursorY - ch, cw, ch, TFT_YELLOW); }
        else if (designerSelectedType == OBST_COIN) { cw = 10; ch = 10; canvas.fillCircle(curScreenX + cw/2, designerCursorY - ch/2, cw/2, TFT_BLUE); }
        else if (designerSelectedType == OBST_PORTAL) { cw = 20; ch = 40; canvas.fillRoundRect(curScreenX, designerCursorY - ch, cw, ch, 5, TFT_MAGENTA); }
        
        if (designerSelectedType == OBST_COIN) canvas.drawCircle(curScreenX + cw/2, designerCursorY - ch/2, cw/2, TFT_WHITE);
        else canvas.drawRect(curScreenX, designerCursorY - ch, cw, ch, TFT_WHITE);
        
        int mmH = 12;
        int mmW = SCREEN_WIDTH;
        float levelWidth = std::max(currentLevel.finishX, designerCursorX + 500.0f); 
        float mmScaleX = (float)mmW / levelWidth;
        float mmScaleY = (float)mmH / SCREEN_HEIGHT;

        canvas.fillRect(0, 0, mmW, mmH, canvas.color565(30, 30, 30));
        canvas.drawFastHLine(0, mmH, mmW, TFT_WHITE);

        for (const auto& obj : currentLevel.objects) {
            int mmX = (int)(obj.x * mmScaleX);
            int mmY = (int)((obj.y - obj.h) * mmScaleY); 
            int mmOw = std::max(1, (int)(obj.w * mmScaleX));
            int mmOh = std::max(1, (int)(obj.h * mmScaleY));
            
            uint16_t col = TFT_LIGHTGREY;
            if (obj.type == OBST_SPIKE) col = TFT_RED;
            else if (obj.type == OBST_BLOCK) col = TFT_ORANGE;
            else if (obj.type == OBST_PAD) col = TFT_YELLOW;
            else if (obj.type == OBST_COIN) col = TFT_BLUE;
            else if (obj.type == OBST_PORTAL) col = TFT_MAGENTA;
            
            canvas.fillRect(mmX, mmY, mmOw, mmOh, col);
        }

        int mmFinishX = (int)(currentLevel.finishX * mmScaleX);
        if (mmFinishX < mmW) canvas.drawFastVLine(mmFinishX, 0, mmH, TFT_WHITE);

        int mmViewX = (int)(cameraX * mmScaleX);
        int mmViewW = (int)(SCREEN_WIDTH * mmScaleX);
        canvas.drawRect(mmViewX, 0, mmViewW, mmH, TFT_CYAN);
        
        int mmCursorX = (int)(designerCursorX * mmScaleX);
        canvas.drawFastVLine(mmCursorX, 0, mmH, TFT_GREEN);

        canvas.setTextDatum(TL_DATUM);
        canvas.setTextColor(TFT_WHITE);
        canvas.setCursor(2, mmH + 2);
#if !defined(ARDUINO)
        canvas.printf("X:%.0f Y:%.0f | O:Cycle P:BG M:BGM Space:Place E:Del X:Save", designerCursorX, designerCursorY);
#else
        canvas.printf("X:%.0f Y:%.0f | O:Cycle P:BG M:BGM Ent:Place E:Del X:Save", designerCursorX, designerCursorY);
#endif
        
        if (millis() - designerMusicTimer < 3000) {
            canvas.setTextDatum(BC_DATUM);
            canvas.setTextColor(TFT_WHITE);
            canvas.drawString(currentLevel.bgMusic.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 5);
        }
        
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

    if (state == SHOP) {
        drawShop();
        canvas.pushSprite(0, 0);
        return;
    }

    if (state == DEATH) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_RED);
        canvas.drawString("YOU DIED", screenWidth / 2, SCREEN_HEIGHT / 2 - 20);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
        char livesBuf[32];
        snprintf(livesBuf, sizeof(livesBuf), "Lives Remaining: %d", playerLives);
        canvas.drawString(livesBuf, screenWidth / 2, SCREEN_HEIGHT / 2 + 5);
        
#if !defined(ARDUINO)
        canvas.drawString("Press Space to Restart", screenWidth / 2, SCREEN_HEIGHT / 2 + 25);
#else
        canvas.drawString("Press OK to Restart", screenWidth / 2, SCREEN_HEIGHT / 2 + 25);
#endif
        canvas.pushSprite(0, 0);
        return;
    }

    drawParallaxBackground();

    uint16_t themeColor = hslToRgb565(colorHue, 0.8f, 0.5f);
    uint16_t spikeColor = hslToRgb565(fmodf(colorHue + 30.0f, 360.0f), 0.9f, 0.45f);
    uint16_t blockColor = hslToRgb565(fmodf(colorHue + 60.0f, 360.0f), 0.7f, 0.4f);
    uint16_t padColor   = hslToRgb565(fmodf(colorHue + 120.0f, 360.0f), 0.9f, 0.55f);
    uint16_t coinColor  = hslToRgb565(fmodf(colorHue + 180.0f, 360.0f), 0.8f, 0.55f);
    uint16_t floorColor = hslToRgb565(colorHue, 0.5f, 0.35f);

    canvas.drawFastHLine(0, FLOOR_Y, screenWidth, floorColor);
    canvas.drawFastHLine(0, FLOOR_Y - 1, screenWidth, hslToRgb565(colorHue, 0.3f, 0.2f));

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
            canvas.fillTriangle(x1, y1, x2, y2, x3, y3, spikeColor);
        } else if (obj.type == OBST_BLOCK) {
            canvas.fillRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, blockColor);
            canvas.drawRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, themeColor);
        } else if (obj.type == OBST_PAD) {
            canvas.fillRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, padColor);
        } else if (obj.type == OBST_COIN) {
            if (obj.active) {
                canvas.fillCircle((int)screenX + obj.w / 2, (int)obj.y - obj.h / 2, obj.w / 2, coinColor);
                canvas.drawCircle((int)screenX + obj.w / 2, (int)obj.y - obj.h / 2, obj.w / 2 + 1, themeColor);
            }
        } else if (obj.type == OBST_PORTAL) {
            canvas.fillRoundRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, 10, TFT_MAGENTA);
            canvas.drawRoundRect((int)screenX, (int)obj.y - obj.h, obj.w, obj.h, 10, TFT_CYAN);
            canvas.drawEllipse((int)screenX + obj.w/2, (int)obj.y - obj.h/2, obj.w/2 - 2, obj.h/2 - 4, TFT_WHITE);
        }
    }

    float finishScreenX = currentLevel.finishX - cameraX;
    if (finishScreenX >= 0 && finishScreenX < SCREEN_WIDTH) {
        for (int i = 0; i < SCREEN_HEIGHT; i += 10) {
            canvas.fillRect((int)finishScreenX, i, 10, 10, (i / 10 % 2 == 0) ? TFT_WHITE : TFT_BLACK);
        }
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (trails[i].life > 0) {
            float scale = (trails[i].life / 30.0f) * trails[i].size;
            cubeSprite.pushRotateZoom(&canvas, trails[i].x + CUBE_SIZE / 2.0f, trails[i].y + CUBE_SIZE / 2.0f, trails[i].angle, scale, scale);
        }
    }

    for (int i = 0; i < MAX_BURST; i++) {
        if (bursts[i].life > 0) {
            canvas.fillRect((int)bursts[i].x, (int)bursts[i].y, 2, 2, bursts[i].color);
        }
    }

    for (int g = 0; g < NUM_GHOSTS; g++) {
        int idx = (ghostIdx + g) % NUM_GHOSTS;
        float alpha = (float)(g + 1) / (NUM_GHOSTS + 2); 
        float scale = 0.5f + alpha * 0.4f;
        cubeSprite.pushRotateZoom(&canvas,
            ghostTrail[idx].x + CUBE_SIZE / 2.0f,
            ghostTrail[idx].y + CUBE_SIZE / 2.0f,
            ghostTrail[idx].angle, scale, scale);
    }

    float scaleX = (playerMode == MODE_SHIP) ? 1.3f : 1.0f;
    float scaleY = (playerMode == MODE_SHIP) ? 0.8f : 1.0f;
    cubeSprite.pushRotateZoom(&canvas, CUBE_X + CUBE_SIZE / 2.0f, cubeY + CUBE_SIZE / 2.0f, cubeAngle, scaleX, scaleY);

    canvas.setTextSize(1);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(5, 5);
    int progress = (int)(cameraX * 100 / currentLevel.finishX);
    if (progress > 100) progress = 100;
    canvas.printf("Level %d: %d%% | Coins: %d | Lives: %d", currentLevelIndex, progress, playerCoins + score, playerLives);

    int px = SCREEN_WIDTH - 25;
    if (playerInventory.hasDoubleJump) {
        loadIcon("double_jump.png");
        iconSprite.pushSprite(px, 5);
        px -= 22;
    }
    if (playerInventory.hasBigJump) {
        loadIcon("big_jump.png");
        iconSprite.pushSprite(px, 5);
        px -= 22;
    }
    if (playerInventory.shieldHits > 0) {
        loadIcon("shield.png");
        iconSprite.pushSprite(px, 5);
        canvas.setTextDatum(TR_DATUM);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawNumber(playerInventory.shieldHits, px + 20, 20);
        px -= 22;
    }

    if (state == GAMEOVER) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_RED);
        canvas.drawString("GAME OVER", screenWidth / 2, SCREEN_HEIGHT / 2 - 10);
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        canvas.drawString("Press Space to Continue", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
#else
        canvas.drawString("Press OK to Continue", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
#endif
        canvas.setTextDatum(TL_DATUM);
    }

    canvas.pushSprite(0, 0);
}
