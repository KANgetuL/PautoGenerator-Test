#include "Generator.h"
#include <fstream>
#include <random>
#include <iomanip>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <set>
#include <map>

using json = nlohmann::json;
namespace fs = std::filesystem;

Generator::Generator(float bpm, const std::string& bgPath, const std::string& mp3Path)
    : bpm_(std::round(bpm * 10.0f) / 10.0f),
    bgPath_(bgPath),
    mp3Path_(mp3Path) {
    id_ = generateId();
    outJpg_ = id_ + ".jpg";
    outOgg_ = id_ + ".ogg";
    outJson_ = id_ + ".json";
}

std::string Generator::generateId() {
    std::mt19937_64 rng{ std::random_device{}() };
    std::uniform_int_distribution<uint64_t> dist(10000000, 99999999);
    return std::to_string(dist(rng));
}

bool Generator::copyBackground() {
    try {
        fs::copy_file(bgPath_, outJpg_, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error copying background: " << e.what() << '\n';
        return false;
    }
}

bool Generator::convertSong() {
    std::string cmd = "ffmpeg -y -i \"" + mp3Path_ + "\" \"" + outOgg_ + "\"";
    return std::system(cmd.c_str()) == 0;
}

bool Generator::buildAndWriteJson() {
    json j;
    j["BPMList"] = json::array();
    j["META"] = {
        {"RPEVersion", 160},
        {"background", outJpg_},
        {"charter", "0"},
        {"composer", "0"},
        {"id", id_},
        {"level", "0"},
        {"name", "0"},
        {"offset", 0},
        {"song", outOgg_}
    };

    j["judgeLineGroup"] = { "Default" };
    json judgeLine = {
        {"Group", 0},
        {"Name", "Untitled"},
        {"Texture", "line.png"},
        {"alphaControl", json::array({
            {{"alpha", 1.0}, {"easing", 1}, {"x", 0.0}},
            {{"alpha", 1.0}, {"easing", 1}, {"x", 9999999.0}}
        })},
        {"bpmfactor", 1.0},
        {"eventLayers", json::array({
            {
                {"alphaEvents", json::array({
                    {
                        {"easingLeft", 0.0}, {"easingRight", 1.0}, {"easingType", 1},
                        {"end", 255}, {"endTime", {1, 0, 1}},
                        {"linkgroup", 0}, {"start", 255}, {"startTime", {0, 0, 1}}
                    }
                })},
                {"moveXEvents", json::array({
                    {
                        {"easingLeft", 0.0}, {"easingRight", 1.0}, {"easingType", 1},
                        {"end", 0.0}, {"endTime", {1, 0, 1}},
                        {"linkgroup", 0}, {"start", 0.0}, {"startTime", {0, 0, 1}}
                    }
                })},
                {"moveYEvents", json::array({
                    {
                        {"easingLeft", 0.0}, {"easingRight", 1.0}, {"easingType", 1},
                        {"end", -300.0}, {"endTime", {1, 0, 1}},
                        {"linkgroup", 0}, {"start", -300.0}, {"startTime", {0, 0, 1}}
                    }
                })},
                {"rotateEvents", json::array({
                    {
                        {"easingLeft", 0.0}, {"easingRight", 1.0}, {"easingType", 1},
                        {"end", 0.0}, {"endTime", {1, 0, 1}},
                        {"linkgroup", 0}, {"start", 0.0}, {"startTime", {0, 0, 1}}
                    }
                })},
                {"speedEvents", json::array()}
            }
        })},
        {"extended", {
            {"inclineEvents", json::array({
                {
                    {"easingLeft", 0.0}, {"easingRight", 1.0}, {"easingType", 0},
                    {"end", 0.0}, {"endTime", {1, 0, 1}},
                    {"linkgroup", 0}, {"start", 0.0}, {"startTime", {0, 0, 1}}
                }
            })}
        }},
        {"father", -1},
        {"isCover", 1},
        {"notes", json::array()},
        {"numOfNotes", 0},
        {"posControl", json::array({
            {{"easing", 1}, {"pos", 1.0}, {"x", 0.0}},
            {{"easing", 1}, {"pos", 1.0}, {"x", 9999999.0}}
        })},
        {"sizeControl", json::array({
            {{"easing", 1}, {"size", 1.0}, {"x", 0.0}},
            {{"easing", 1}, {"size", 1.0}, {"x", 9999999.0}}
        })},
        {"skewControl", json::array({
            {{"easing", 1}, {"skew", 0.0}, {"x", 0.0}},
            {{"easing", 1}, {"skew", 0.0}, {"x", 9999999.0}}
        })},
        {"yControl", json::array({
            {{"easing", 1}, {"x", 0.0}, {"y", 1.0}},
            {{"easing", 1}, {"x", 9999999.0}, {"y", 1.0}}
        })},
        {"zOrder", 0}
    };

    j["judgeLineList"] = json::array({ judgeLine });

    std::ofstream ofs(outJson_);
    if (!ofs) {
        std::cerr << "Failed to open JSON file for writing: " << outJson_ << '\n';
        return false;
    }
    ofs << j.dump(4);
    return true;
}

json Generator::loadJson() const {
    std::ifstream ifs(outJson_);
    if (!ifs) {
        throw std::runtime_error("Failed to open JSON file: " + outJson_);
    }
    return json::parse(ifs);
}

void Generator::saveJson(const json& j) const {
    std::ofstream ofs(outJson_);
    if (!ofs) {
        throw std::runtime_error("Failed to save JSON file: " + outJson_);
    }
    ofs << j.dump(4);
}

bool Generator::generateNotes() {
    try {
        // 1. 加载音频
        auto audio = AudioProcessor::LoadOgg(outOgg_);

        // 2. 检测BPM变化（使用15秒窗口，5秒步长）
        auto bpmChanges = AudioProcessor::DetectBPMChanges(audio, 15.0f, 5.0f);

        // 3. 检测节拍点
        auto beats = AudioProcessor::DetectBeats(audio);
        if (beats.empty()) {
            throw std::runtime_error("No beats detected in the audio");
        }

        // 4. 加载JSON
        json j = loadJson();
        json& judgeLine = j["judgeLineList"][0];
        json& notes = judgeLine["notes"];

        // 5. 更新BPMList
        json& bpmList = j["BPMList"];
        bpmList = json::array();

        // 添加初始BPM（0时刻）
        bpmList.push_back({
            {"bpm", bpmChanges[0].second},
            {"startTime", BeatCalculator::BeatToTriplet(0)}
            });

        // 添加检测到的BPM变化点
        for (const auto& change : bpmChanges) {
            if (change.first > 0) { // 跳过0时刻
                float beat = BeatCalculator::TimeToBeat(change.first, change.second);
                bpmList.push_back({
                    {"bpm", change.second},
                    {"startTime", BeatCalculator::BeatToTriplet(beat)}
                    });
            }
        }

        // 6. 生成音符（考虑BPM变化）
        const std::vector<float> positions = { 390.0f, 130.0f, -130.0f, -390.0f };
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 3);
        std::map<float, std::set<float>> beatPositions;

        // 为每个节拍点找到对应的BPM
        std::vector<float> beatValues;
        for (float time : beats) {
            // 找到当前时间对应的BPM
            float currentBPM = bpmChanges[0].second;
            for (const auto& change : bpmChanges) {
                if (time >= change.first) {
                    currentBPM = change.second;
                }
                else {
                    break;
                }
            }

            // 转换时间到节拍
            float beat = BeatCalculator::TimeToBeat(time, currentBPM);
            beatValues.push_back(beat);
        }

        // 生成音符
        for (size_t i = 0; i < beatValues.size(); i++) {
            float beat = beatValues[i];

            // 确定可用的位置
            std::vector<float> availablePos = positions;
            if (beatPositions.find(beat) != beatPositions.end()) {
                auto& used = beatPositions[beat];
                availablePos.erase(
                    std::remove_if(availablePos.begin(), availablePos.end(),
                        [&](float pos) { return used.find(pos) != used.end(); }),
                    availablePos.end()
                );
            }

            if (availablePos.empty()) continue;

            // 随机选择位置
            int index = dist(rng) % availablePos.size();
            float pos = availablePos[index];

            // 添加到已使用位置
            beatPositions[beat].insert(pos);

            // 转换为三元组
            auto triplet = BeatCalculator::BeatToTriplet(beat);

            // 创建音符
            json note = {
                {"above", 1},
                {"alpha", 255},
                {"endTime", triplet},
                {"isFake", 0},
                {"positionX", pos},
                {"size", 1.0},
                {"speed", 1.0},
                {"startTime", triplet},
                {"type", 1},
                {"visibleTime", 999999.0},
                {"yOffset", 0.0}
            };

            notes.push_back(note);
        }

        // 7. 更新音符数量
        judgeLine["numOfNotes"] = notes.size();

        // 8. 保存JSON
        saveJson(j);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Note generation failed: " << e.what() << '\n';
        return false;
    }
}

bool Generator::processStep1() {
    if (!copyBackground()) {
        std::cerr << "Error: copy background failed\n";
        return false;
    }
    if (!convertSong()) {
        std::cerr << "Error: convert song failed\n";
        return false;
    }
    if (!buildAndWriteJson()) {
        std::cerr << "Error: write JSON failed\n";
        return false;
    }
    return true;
}

bool Generator::processStep2() {
    return generateNotes();
}