#include <iostream>
#include "Generator.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: PautoGenerator-Test <bpm> <background.jpg> <song.mp3>\n";
        return 1;
    }
    float bpm = std::stof(argv[1]);
    Generator gen(bpm, argv[2], argv[3]);
    if (!gen.processStep1()) {
        return 1;
    }
    std::cout << "Step1 complete. Generated files:\n"
        << "  " << gen.getJsonName() << "\n"
        << "  " << gen.getJpgName() << "\n"
        << "  " << gen.getOggName() << "\n";
    return 0;
}