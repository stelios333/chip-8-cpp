#pragma once
#include <SDL2/SDL.h>

class ToneGenerator {
private:
    SDL_AudioDeviceID device = 0;
    struct AudioData {
        double phase = 0.0;
        double phase_increment = 0.0;
        bool* playing = nullptr;
    } audio_data;

    static void audio_callback(void* userdata, Uint8* stream, int len);

public:
    ToneGenerator();
    ~ToneGenerator();
    void setFrequency(int freq);
    void play();
    void stop();
    bool playing = false;
};