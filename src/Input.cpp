#include "Input.h"
#include "Globals.h"
#include <string.h>

#if !defined(ARDUINO)
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

#if defined(ARDUINO)
bool espEdge_enter()     { return espCur.enter && !espPrev.enter; }
bool espEdge_del()       { return espCur.del   && !espPrev.del; }
bool espEdge_space()     { return espCur.space && !espPrev.space; }
bool espEdge_d()         { return espCur.key_d && !espPrev.key_d; }
bool espEdge_e()         { return espCur.key_e && !espPrev.key_e; }
bool espEdge_o()         { return espCur.key_o && !espPrev.key_o; }
bool espEdge_x()         { return espCur.key_x && !espPrev.key_x; }
bool espEdge_s()         { return espCur.key_s && !espPrev.key_s; }
bool espEdge_up()        { return espCur.key_semicolon && !espPrev.key_semicolon; }
bool espEdge_down()      { return espCur.key_period    && !espPrev.key_period; }
bool espEdge_left()      { return espCur.key_left  && !espPrev.key_left; }
bool espEdge_right()     { return espCur.key_right && !espPrev.key_right; }
bool espEdge_p()         { return espCur.key_p && !espPrev.key_p; }
bool espEdge_m()         { return espCur.key_m && !espPrev.key_m; }
bool espEdge_l()         { return espCur.key_l && !espPrev.key_l; }

static uint32_t lastMenuActionMs = 0;
const uint32_t  MENU_DEBOUNCE_MS = 120;
bool menuDebounceOk() {
    uint32_t now = millis();
    if (now - lastMenuActionMs < MENU_DEBOUNCE_MS) return false;
    lastMenuActionMs = now;
    return true;
}
#endif
