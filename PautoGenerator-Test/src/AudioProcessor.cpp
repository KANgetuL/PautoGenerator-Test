#include "AudioProcessor.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <sndfile.h>
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ����������˫���� FFT ����
std::vector<float> ComputeFFT(const std::vector<float>& input) {
    int n = input.size();
    double* in = (double*)fftw_malloc(sizeof(double) * n);
    fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (n / 2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(n, in, out, FFTW_ESTIMATE);

    for (int i = 0; i < n; i++) {
        in[i] = static_cast<double>(input[i]);
    }

    fftw_execute(plan);

    std::vector<float> spectrum(n / 2 + 1);
    for (int i = 0; i < n / 2 + 1; i++) {
        spectrum[i] = static_cast<float>(std::sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]));
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return spectrum;
}

AudioProcessor::AudioData AudioProcessor::LoadOgg(const std::string& filename) {
    SF_INFO sfInfo;
    std::memset(&sfInfo, 0, sizeof(SF_INFO));
    SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &sfInfo);
    if (!file) {
        throw std::runtime_error("Failed to open audio file: " + filename);
    }

    AudioData data;
    data.sampleRate = sfInfo.samplerate;
    data.channels = sfInfo.channels;

    const size_t bufferSize = 4096;
    std::vector<float> buffer(bufferSize * data.channels);

    while (true) {
        sf_count_t count = sf_read_float(file, buffer.data(), bufferSize);
        if (count <= 0) break;

        // �����������ȡƽ��ֵתΪ������
        if (data.channels > 1) {
            for (int i = 0; i < count; i += data.channels) {
                float sum = 0.0f;
                for (int c = 0; c < data.channels; c++) {
                    sum += buffer[i + c];
                }
                data.samples.push_back(sum / data.channels);
            }
        }
        else {
            data.samples.insert(data.samples.end(), buffer.begin(), buffer.begin() + count);
        }
    }

    sf_close(file);
    return data;
}

std::vector<float> AudioProcessor::DetectBeats(const AudioData& audio) {
    std::vector<float> beats;
    const int windowSize = 2048; // FFT���ڴ�С
    const int hopSize = 512;    // ֡��

    // ����ÿ�����ڵ�����
    std::vector<float> energy;
    for (size_t i = 0; i < audio.samples.size() - windowSize; i += hopSize) {
        float sum = 0.0f;
        for (int j = 0; j < windowSize; j++) {
            sum += audio.samples[i + j] * audio.samples[i + j];
        }
        energy.push_back(sum / windowSize);
    }

    // ���������仯��
    std::vector<float> energyDiff(energy.size(), 0.0f);
    for (size_t i = 1; i < energy.size(); i++) {
        energyDiff[i] = energy[i] - energy[i - 1];
    }

    // ����ֵ������ͻ��㣩
    if (energyDiff.empty()) return beats;

    const float maxDiff = *std::max_element(energyDiff.begin(), energyDiff.end());
    if (maxDiff <= 0.0f) return beats;  // ��ֹ������

    const float threshold = 0.2f * maxDiff;

    for (size_t i = 1; i < energyDiff.size() - 1; i++) {
        if (energyDiff[i] > threshold &&
            energyDiff[i] > energyDiff[i - 1] &&
            energyDiff[i] > energyDiff[i + 1]) {
            float time = static_cast<float>(i * hopSize) / audio.sampleRate;
            beats.push_back(time);
        }
    }

    return beats;
}

std::vector<std::pair<float, float>> AudioProcessor::DetectBPMChanges(
    const AudioData& audio,
    float windowSize,
    float step)
{
    auto beats = DetectBeats(audio);
    std::vector<std::pair<float, float>> bpmChanges;

    if (beats.size() < 2) {
        // û���㹻�Ľ��ĵ㣬ʹ�õ������ļ������BPM
        if (!beats.empty()) {
            float avgInterval = beats.back() / (beats.size() - 1);
            bpmChanges.push_back({ 0.0f, 60.0f / avgInterval });
        }
        return bpmChanges;
    }

    // ��ʱ�䴰�ڼ��BPM
    const float totalDuration = static_cast<float>(audio.samples.size()) / audio.sampleRate;
    const int numWindows = static_cast<int>((totalDuration - windowSize) / step) + 1;

    for (int i = 0; i < numWindows; i++) {
        const float windowStart = i * step;
        const float windowEnd = windowStart + windowSize;

        // �ռ������ڵĽ��ļ��
        std::vector<float> intervals;
        float lastBeatInWindow = -1.0f;

        for (size_t j = 0; j < beats.size(); j++) {
            if (beats[j] >= windowStart && beats[j] <= windowEnd) {
                if (lastBeatInWindow >= 0) {
                    intervals.push_back(beats[j] - lastBeatInWindow);
                }
                lastBeatInWindow = beats[j];
            }
        }

        if (intervals.size() >= 3) {
            // ������λ������������쳣ֵӰ��
            std::sort(intervals.begin(), intervals.end());
            float medianInterval = intervals[intervals.size() / 2];
            float bpm = 60.0f / medianInterval;

            // ��¼��������ʱ���BPM
            bpmChanges.push_back({ windowStart + windowSize / 2.0f, bpm });
        }
    }

    // ���û�м�⵽�仯��ʹ��ȫ��BPM
    if (bpmChanges.empty()) {
        float avgInterval = 0.0f;
        for (size_t i = 1; i < beats.size(); i++) {
            avgInterval += beats[i] - beats[i - 1];
        }
        avgInterval /= (beats.size() - 1);
        bpmChanges.push_back({ 0.0f, 60.0f / avgInterval });
    }

    return bpmChanges;
}

std::vector<std::vector<float>> AudioProcessor::GenerateSpectrogram(const AudioData& audio, int windowSize, int hopSize) {
    std::vector<std::vector<float>> spectrogram;

    // Ӧ�ú�����
    std::vector<float> window(windowSize);
    for (int i = 0; i < windowSize; i++) {
        window[i] = 0.5f * (1 - cos(2 * M_PI * i / (windowSize - 1)));
    }

    for (size_t i = 0; i < audio.samples.size() - windowSize; i += hopSize) {
        // Ӧ�ô��ں���
        std::vector<float> frame(windowSize);
        for (int j = 0; j < windowSize; j++) {
            frame[j] = audio.samples[i + j] * window[j];
        }

        // ����FFT
        auto spectrum = ComputeFFT(frame);
        spectrogram.push_back(spectrum);
    }

    return spectrogram;
}

float BeatCalculator::TimeToBeat(float time, float bpm) {
    return time * bpm / 60.0f;
}

std::vector<int> BeatCalculator::BeatToTriplet(float beat) {
    int integer = static_cast<int>(beat);
    float fractional = beat - integer;

    // ��С������ת��Ϊ��ӽ��ķ�������ĸΪ32��
    const int denominator = 32;
    int numerator = static_cast<int>(std::round(fractional * denominator));

    // �����������뵼�µ��������ֽ�λ
    if (numerator == denominator) {
        integer++;
        numerator = 0;
    }

    // Լ��
    int a = numerator;
    int b = denominator;
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    int gcd = a;

    return { integer, numerator / gcd, denominator / gcd };
}