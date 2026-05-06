#include "Audio.h"
#include "Globals.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(ARDUINO)
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
#endif

void stopAudio() {
#if !defined(ARDUINO)
    system("killall -9 afplay 2>/dev/null");
#else
    stopMP3();
#endif
}

void playAudio(const char* filename) {
#if defined(ARDUINO)
    std::string path = "/audio/";
    if (filename[0] == '/') path = filename; // already has path
    else path += filename;
    
    if (!LittleFS.exists(path.c_str())) {
        path = "/audio/bgm.mp3";
    }
    playMP3(path.c_str());
#else
    char path[512];
    if (strstr(filename, "/")) strncpy(path, filename, sizeof(path));
    else snprintf(path, sizeof(path), "audio/%s", filename);

    FILE* f = fopen(path, "r");
    if (!f) {
        snprintf(path, sizeof(path), "audio/bgm.mp3");
    } else {
        fclose(f);
    }
    stopAudio();
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "afplay \"%s\" &", path);
    system(cmd);
#endif
}

void applyVolume() {
    int vol = volumeLevel * 25; // 0-250
    if (vol > 255) vol = 255;
    M5.Speaker.setVolume(vol);
#if defined(ARDUINO)
    if (audio_out) {
        float gain = volumeLevel / 10.0f;
        audio_out->SetGain(gain);
    }
#endif
}
