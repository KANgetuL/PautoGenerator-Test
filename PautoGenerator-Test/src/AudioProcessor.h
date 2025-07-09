#pragma once

#include <vector>
#include <complex>
#include <string>

class AudioProcessor {
public:
    struct AudioData {
        std::vector<float> samples;
        int sampleRate;
        int channels;
    };

    static AudioData LoadOgg(const std::string& filename);
    static std::vector<float> DetectBeats(const AudioData& audio, float bpm);
    static std::vector<std::vector<float>> GenerateSpectrogram(const AudioData& audio, int windowSize = 1024, int hopSize = 512);


    static std::vector<float> DetectBeats(const AudioData& audio);
    static std::vector<std::pair<float, float>> DetectBPMChanges(
        const AudioData& audio,
        float windowSize = 10.0f,  // 窗口大小(秒)
        float step = 1.0f          // 滑动步长(秒)
    );
};

class BeatCalculator {
public:
    static float TimeToBeat(float time, float bpm);
    static std::vector<int> BeatToTriplet(float beat);
};