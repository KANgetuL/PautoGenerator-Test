#include <iostream>
#include "Generator.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: PautoGenerator-Test <bpm> <background.jpg> <song.mp3>\n";
        return 1;
    }

    try {
        float bpm = std::stof(argv[1]);
        Generator gen(bpm, argv[2], argv[3]);

        // 第一步：基础文件生成
        if (!gen.processStep1()) {
            std::cerr << "Step1 failed\n";
            return 1;
        }

        std::cout << "Step1 complete. Generated files:\n"
            << "  " << gen.getJsonName() << "\n"
            << "  " << gen.getJpgName() << "\n"
            << "  " << gen.getOggName() << "\n";

        // 第二步：音符生成
        if (!gen.processStep2()) {
            std::cerr << "Step2 failed\n";
            return 1;
        }

        std::cout << "Step2 complete. Notes generated.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}