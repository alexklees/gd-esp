#pragma once

#include "Config.h"

#if !defined(ARDUINO)
#include <SDL2/SDL.h>

bool isKeyPressed(SDL_Scancode key);
void updateKeyboard();
void postUpdateKeyboard();
#endif

#if defined(ARDUINO)
bool espEdge_enter();
bool espEdge_del();
bool espEdge_space();
bool espEdge_d();
bool espEdge_e();
bool espEdge_o();
bool espEdge_x();
bool espEdge_s();
bool espEdge_up();
bool espEdge_down();
bool espEdge_left();
bool espEdge_right();
bool espEdge_p();
bool espEdge_m();
bool espEdge_l();

bool menuDebounceOk();
#endif
