#include "audio.h"
#include <cmath>
#include <iostream>

const int SAMPLE_RATE = 44100;
const int AMPLITUDE   = 28000;
const double FREQUENCY = 440.0;

void ToneGenerator::audio_callback(void* userdata, Uint8* stream, int len) {
    AudioData* data = static_cast<AudioData*>(userdata);
    Sint16* buffer = reinterpret_cast<Sint16*>(stream);
    int samples = len / sizeof(Sint16);  // number of samples (mono)
    
    for (int i = 0; i < samples; ++i) {
        if (data->playing && *data->playing) {
            buffer[i] = static_cast<Sint16>(AMPLITUDE * std::sin(data->phase));
            data->phase += data->phase_increment;

            // Keep phase in [0, 2π) to avoid floating-point drift
            if (data->phase >= 2.0 * M_PI) {
                data->phase -= 2.0 * M_PI;
            }
        } else {
            buffer[i] = 0;
        }
    }
}

void ToneGenerator::setFrequency(int freq) {
    audio_data.phase_increment = 2.0 * M_PI * freq / SAMPLE_RATE;
}

ToneGenerator::ToneGenerator() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return;
    }

    audio_data.phase_increment = 2.0 * M_PI * FREQUENCY / SAMPLE_RATE;
    audio_data.playing = &playing;
    
    SDL_AudioSpec want{}, have{};
    want.freq     = SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 4096;
    want.callback = audio_callback;
    want.userdata = &audio_data;     // now points to a member → lives as long as the object

    device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);  // assign to the MEMBER

    if (device == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }
    SDL_PauseAudioDevice(device, 0);
}

ToneGenerator::~ToneGenerator() {
    if (device) {
        SDL_CloseAudioDevice(device);
    }}

void ToneGenerator::play() {
    playing = true;
}

void ToneGenerator::stop() {
    playing = false;

}