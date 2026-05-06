#pragma once

#include "Config.h"

#if defined(ARDUINO)
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

extern AudioGeneratorMP3 *mp3_gen;
extern AudioFileSourceLittleFS *audio_file;
extern AudioOutputI2S *audio_out;

void stopMP3();
void playMP3(const char* path);
#endif

void stopAudio();
void playAudio(const char* filename);
void applyVolume();
