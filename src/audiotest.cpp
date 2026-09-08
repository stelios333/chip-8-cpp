#include "audio.h"
#include <iostream>

ToneGenerator audio;

int main() {
    audio.play();
    std::cin.get();
    audio.stop();
    std::cin.get();
    return 0;
}
