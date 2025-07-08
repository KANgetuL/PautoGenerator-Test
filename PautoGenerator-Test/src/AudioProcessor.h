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
};

class BeatCalculator {
public:
    static float TimeToBeat(float time, float bpm);
    static std::vector<int> BeatToTriplet(float beat);
};