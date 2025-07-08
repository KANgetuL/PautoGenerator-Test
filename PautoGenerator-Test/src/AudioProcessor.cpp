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

// 辅助函数：FFT计算
std::vector<float> ComputeFFT(const std::vector<float>& input) {
    int n = input.size();
    fftwf_complex* out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * (n / 2 + 1));
    float* in = (float*)fftwf_malloc(sizeof(float) * n);

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(n, in, out, FFTW_ESTIMATE);

    std::copy(input.begin(), input.end(), in);
    fftwf_execute(plan);

    std::vector<float> spectrum(n / 2 + 1);
    for (int i = 0; i < n / 2 + 1; i++) {
        spectrum[i] = std::sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    }

    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    return spectrum;
}

AudioProcessor::AudioData AudioProcessor::LoadOgg(const std::string& filename) {
    SF_INFO sfInfo;
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

        // 如果多声道，取平均值转为单声道
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

std::vector<float> AudioProcessor::DetectBeats(const AudioData& audio, float bpm) {
    std::vector<float> beats;
    const float beatInterval = 60.0f / bpm; // 节拍间隔（秒）
    const int windowSize = 2048; // FFT窗口大小
    const int hopSize = 512;    // 帧移

    // 计算每个窗口的能量
    std::vector<float> energy;
    for (size_t i = 0; i < audio.samples.size() - windowSize; i += hopSize) {
        float sum = 0.0f;
        for (int j = 0; j < windowSize; j++) {
            sum += audio.samples[i + j] * audio.samples[i + j];
        }
        energy.push_back(sum / windowSize);
    }

    // 计算能量变化率
    std::vector<float> energyDiff(energy.size(), 0.0f);
    for (size_t i = 1; i < energy.size(); i++) {
        energyDiff[i] = energy[i] - energy[i - 1];
    }

    // 检测峰值（能量突变点）
    const float threshold = 0.2f * (*std::max_element(energyDiff.begin(), energyDiff.end()));
    for (size_t i = 1; i < energyDiff.size() - 1; i++) {
        if (energyDiff[i] > threshold && energyDiff[i] > energyDiff[i - 1] && energyDiff[i] > energyDiff[i + 1]) {
            float time = static_cast<float>(i * hopSize) / audio.sampleRate;
            beats.push_back(time);
        }
    }

    return beats;
}

std::vector<std::vector<float>> AudioProcessor::GenerateSpectrogram(const AudioData& audio, int windowSize, int hopSize) {
    std::vector<std::vector<float>> spectrogram;

    // 应用汉宁窗
    std::vector<float> window(windowSize);
    for (int i = 0; i < windowSize; i++) {
        window[i] = 0.5f * (1 - cos(2 * M_PI * i / (windowSize - 1)));
    }

    for (size_t i = 0; i < audio.samples.size() - windowSize; i += hopSize) {
        // 应用窗口函数
        std::vector<float> frame(windowSize);
        for (int j = 0; j < windowSize; j++) {
            frame[j] = audio.samples[i + j] * window[j];
        }

        // 计算FFT
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

    // 将小数部分转换为最接近的分数（分母为32）
    const int denominator = 32;
    int numerator = static_cast<int>(std::round(fractional * denominator));

    // 处理四舍五入导致的整数部分进位
    if (numerator == denominator) {
        integer++;
        numerator = 0;
    }

    // 约分
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