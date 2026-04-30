#if !defined(ARDUINO)
#include <SDL2/SDL.h>
#endif
#include <M5Unified.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

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
const int FLOOR_Y = 110;
const int CUBE_SIZE = 20;
const int CUBE_X = 40;

const float GRAVITY = 0.5f;
const float JUMP_VELOCITY = -7.5f;

// Game state
enum GameState { WELCOME, PLAYING, GAMEOVER, ENTER_NAME };
GameState state = WELCOME;

struct HighScore {
    char name[4];
    int score;
};
const int NUM_HIGH_SCORES = 5;
HighScore highScores[NUM_HIGH_SCORES];

void loadHighScores() {
    FILE* f = fopen("scores.dat", "rb");
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
    FILE* f = fopen("scores.dat", "wb");
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

enum ObstacleType { OBST_SPIKE, OBST_BLOCK, OBST_PAD };
struct Obstacle {
    float x;
    int w, h;
    ObstacleType type;
    bool active;
};

Obstacle obstacle = { SCREEN_WIDTH, 16, 20, OBST_SPIKE, true };
float obstacleSpeed = 3.5f;
float gapMultiplier = 1.0f;

uint32_t lastFrameTime = 0;

void resetGame() {
    cubeY = FLOOR_Y - CUBE_SIZE;
    cubeVelocityY = 0;
    isJumping = false;
    cubeAngle = 0.0f;
    obstacle.x = SCREEN_WIDTH;
    obstacle.active = true;
    int r = rand() % 3;
    obstacle.type = (r == 0) ? OBST_SPIKE : ((r == 1) ? OBST_BLOCK : OBST_PAD);
    if (obstacle.type == OBST_BLOCK) {
        obstacle.w = 20;
        obstacle.h = 20;
    } else if (obstacle.type == OBST_SPIKE) {
        obstacle.w = 16;
        obstacle.h = 20;
    } else {
        obstacle.w = 30;
        obstacle.h = 5;
    }
    score = 0;
    obstacleSpeed = 3.5f;
    gapMultiplier = 1.0f;
    state = PLAYING;
    for (int i = 0; i < MAX_PARTICLES; i++) trails[i].life = 0;
#if !defined(ARDUINO)
    stopAudio();
    system("afplay bgm.mp3 &");
#endif
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    
    loadHighScores();

#if !defined(ARDUINO)
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
    
#if !defined(ARDUINO)
    // Attempt to load cat.png into the sprite (0, 0)
    cubeSprite.drawPngFile("cat.png", 0, 0, CUBE_SIZE, CUBE_SIZE);
#endif

    cubeSprite.setPivot(CUBE_SIZE / 2.0f, CUBE_SIZE / 2.0f);
}

bool checkCollision() {
    if (!obstacle.active) return false;
    if (obstacle.type == OBST_PAD) return false;

    // Spike: simple box collision
    if (obstacle.type == OBST_SPIKE) {
        if (CUBE_X + 2 < obstacle.x + obstacle.w &&
            CUBE_X + CUBE_SIZE - 2 > obstacle.x &&
            cubeY + 2 < FLOOR_Y &&
            cubeY + CUBE_SIZE - 2 > FLOOR_Y - obstacle.h) {
            return true;
        }
    } else if (obstacle.type == OBST_BLOCK) {
        // Block: Collision from side is death
        // We check if we are NOT on top of it
        bool withinX = (CUBE_X + CUBE_SIZE - 2 > obstacle.x && CUBE_X + 2 < obstacle.x + obstacle.w);
        bool hittingSide = (cubeY + CUBE_SIZE - 2 > FLOOR_Y - obstacle.h);
        
        // If hitting side and not falling onto it
        if (withinX && hittingSide && (cubeY + CUBE_SIZE > FLOOR_Y - obstacle.h + 5)) {
            return true;
        }
    }
    return false;
}

void update() {
    M5.update();
    
    if (state == WELCOME) {
        if (M5.BtnA.wasPressed()) {
            resetGame();
        }
        return;
    }
    
    if (state == ENTER_NAME) {
#if !defined(ARDUINO)
        updateKeyboard();
        
        // Check A-Z
        for (int i = SDL_SCANCODE_A; i <= SDL_SCANCODE_Z; i++) {
            if (isKeyPressed((SDL_Scancode)i)) {
                if (nameCharIndex < 3) {
                    currentName[nameCharIndex] = 'A' + (i - SDL_SCANCODE_A);
                    nameCharIndex++;
                }
            }
        }
        
        // Check Backspace
        if (isKeyPressed(SDL_SCANCODE_BACKSPACE)) {
            if (nameCharIndex > 0) {
                nameCharIndex--;
                currentName[nameCharIndex] = ' ';
            }
        }
        
        // Check Enter
        if (isKeyPressed(SDL_SCANCODE_RETURN)) {
            if (nameCharIndex == 3) {
                strcpy(highScores[newHighScoreIndex].name, currentName);
                saveHighScores();
                state = WELCOME;
            }
        }
        
        postUpdateKeyboard();
#else
        uint32_t currentMs = millis();
        if (M5.BtnA.wasPressed()) {
            if (currentName[nameCharIndex] == ' ') currentName[nameCharIndex] = 'A';
            else currentName[nameCharIndex]++;
            if (currentName[nameCharIndex] > 'Z') {
                currentName[nameCharIndex] = 'A';
            }
            lastCharChangeTime = currentMs;
        }
        
        if (currentMs - lastCharChangeTime > 1500 && currentName[nameCharIndex] != ' ') {
            nameCharIndex++;
            if (nameCharIndex >= 3) {
                strcpy(highScores[newHighScoreIndex].name, currentName);
                saveHighScores();
                state = WELCOME;
            } else {
                lastCharChangeTime = currentMs;
            }
        }
#endif
        return;
    }

    bool jumpPressed = false;
    
    // G0 button / BtnA on Cardputer
    if (M5.BtnA.isPressed()) {
        jumpPressed = true;
    }
    
    if (state == GAMEOVER) {
        if (M5.BtnA.wasPressed()) { // Only restart on fresh press
            state = WELCOME;
        }
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
    
    // Block landing logic
    if (obstacle.active && obstacle.type == OBST_BLOCK) {
        if (CUBE_X + CUBE_SIZE > obstacle.x + 2 && CUBE_X < obstacle.x + obstacle.w - 2) {
            if (cubeY + CUBE_SIZE <= FLOOR_Y - obstacle.h + 10 && cubeY + CUBE_SIZE >= FLOOR_Y - obstacle.h - 5) {
                groundLevel = FLOOR_Y - obstacle.h;
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
    if (obstacle.active && obstacle.type == OBST_PAD) {
        bool withinX = (CUBE_X + CUBE_SIZE - 2 > obstacle.x && CUBE_X + 2 < obstacle.x + obstacle.w);
        bool touchingPad = (cubeY + CUBE_SIZE >= FLOOR_Y - obstacle.h);
        if (withinX && touchingPad && cubeVelocityY >= 0) {
            cubeVelocityY = JUMP_VELOCITY;
            isJumping = true;
            M5.Speaker.tone(800, 50, 1);
        }
    }

    // Update obstacles
    if (obstacle.active) {
        obstacle.x -= obstacleSpeed;
        if (obstacle.x + obstacle.w < 0) {
            score++; // Increase score
            
            int diffLevel = score / 10;
            if (score % 10 == 0) {
                obstacleSpeed *= 1.05f;
                gapMultiplier *= 0.90f;
            }
            
            // Gap logic: base gap gets smaller, variance gets larger
            int baseGap = (int)(40 * gapMultiplier);
            int varGap = 50 + (diffLevel * 100); 
            obstacle.x = M5.Display.width() + baseGap + (rand() % varGap);
            obstacle.active = true;
            
            int r = rand() % 3;
            obstacle.type = (r == 0) ? OBST_SPIKE : ((r == 1) ? OBST_BLOCK : OBST_PAD);
            
            if (obstacle.type == OBST_BLOCK) {
                int sizeVar = std::min(15, diffLevel * 5);
                obstacle.w = 20 + (rand() % (sizeVar + 1));
                obstacle.h = 20 + (rand() % (sizeVar + 1));
            } else if (obstacle.type == OBST_SPIKE) {
                int sizeVar = std::min(15, diffLevel * 3);
                obstacle.w = 16 + (rand() % (sizeVar / 2 + 1));
                obstacle.h = 20 + (rand() % (sizeVar + 1));
            } else { // OBST_PAD
                obstacle.w = 30;
                obstacle.h = 5;
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
        for (int i = 0; i < NUM_HIGH_SCORES; i++) {
            if (score > highScores[i].score) {
                newHighScoreIndex = i;
                break;
            }
        }
        
        if (newHighScoreIndex != -1) {
            for (int i = NUM_HIGH_SCORES - 1; i > newHighScoreIndex; i--) {
                highScores[i] = highScores[i-1];
            }
            highScores[newHighScoreIndex].score = score;
            strcpy(highScores[newHighScoreIndex].name, "   ");
            strcpy(currentName, "   ");
            nameCharIndex = 0;
            lastCharChangeTime = millis();
            state = ENTER_NAME;
        } else {
            state = GAMEOVER;
        }
    }
}

void draw() {
    canvas.clear(TFT_BLACK);

    int screenWidth = M5.Display.width();

    if (state == WELCOME) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_CYAN);
        canvas.drawString("Snoopy (Poopy) Dash", screenWidth / 2, SCREEN_HEIGHT / 4 - 10);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
        if ((millis() / 500) % 2 == 0) {
            canvas.drawString("Press OK to Start", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
        }
        
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
    
    if (state == ENTER_NAME) {
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawString("NEW HIGH SCORE!", screenWidth / 2, SCREEN_HEIGHT / 4 - 10);
        
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
#if !defined(ARDUINO)
        canvas.drawString("Type name, Enter to save.", screenWidth / 2, SCREEN_HEIGHT / 4 + 15);
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

    // Draw floor
    canvas.drawLine(0, FLOOR_Y, screenWidth, FLOOR_Y, TFT_WHITE);

    // Draw obstacles
    if (obstacle.active) {
        if (obstacle.type == OBST_SPIKE) {
            int x1 = (int)obstacle.x;
            int y1 = FLOOR_Y;
            int x2 = (int)obstacle.x + (obstacle.w / 2);
            int y2 = FLOOR_Y - obstacle.h;
            int x3 = (int)obstacle.x + obstacle.w;
            int y3 = FLOOR_Y;
            canvas.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_RED);
        } else if (obstacle.type == OBST_BLOCK) {
            canvas.fillRect((int)obstacle.x, FLOOR_Y - obstacle.h, obstacle.w, obstacle.h, TFT_ORANGE);
            canvas.drawRect((int)obstacle.x, FLOOR_Y - obstacle.h, obstacle.w, obstacle.h, TFT_WHITE);
        } else if (obstacle.type == OBST_PAD) {
            canvas.fillRect((int)obstacle.x, FLOOR_Y - obstacle.h, obstacle.w, obstacle.h, TFT_YELLOW);
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

    // Draw score
    canvas.setTextSize(1);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(5, 5);
    canvas.printf("Score: %d", score);

    // Draw Game Over overlay
    if (state == GAMEOVER) {
        canvas.setTextDatum(MC_DATUM); // Middle center
        canvas.setTextSize(2);
        canvas.setTextColor(TFT_RED);
        canvas.drawString("GAME OVER", screenWidth / 2, SCREEN_HEIGHT / 2 - 10);
        canvas.setTextSize(1);
        canvas.setTextColor(TFT_WHITE);
        canvas.drawString("Press OK to Restart", screenWidth / 2, SCREEN_HEIGHT / 2 + 10);
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
