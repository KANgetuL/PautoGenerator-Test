#include "Generator.h"
#include<fstream>
#include<random>
#include<iomanip>
#include<cstdlib>
#include<filesystem>
#include<iostream>

using json=nlohmann::json;
namespace fs=std::filesystem;

Generator::Generator(float bpm, const std::string& bgPath, const std::string& mp3Path)
	:bpm_(std::round(bpm * 10.0f) / 10.0f),
	bgPath_(bgPath),
	mp3Path_(mp3Path) {
	id_ = generateId();
	outJpg_ = id_ + ".jpg";
	outOgg_ = id_ + ".ogg";
	outJson_ = id_ + ".json";

}

std::string Generator::generateId() {
	std::mt19937_64 rng{
		std::random_device{}()
	};
	std::uniform_int_distribution<uint64_t>dist(10000000, 99999999);

	return std::to_string(dist(rng));
}

bool Generator::copyBackground() {
    try {
        //lky修正：使用 fs::copy_options::overwrite_existing
        fs::copy_file(bgPath_, outJpg_, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error copying background: " << e.what() << '\n';
        return false;
    }
}

bool Generator::convertSong() {
	// 依赖本地已安装 ffmpeg
    std::string cmd = "ffmpeg -y -i \"" + mp3Path_ + "\" \"" + outOgg_ + "\"";
	return std::system(cmd.c_str()) == 0;
}

bool Generator::buildAndWriteJson() {
    json j;
    j["BPMList"] = json::array({
        {
            {"bpm", bpm_},
            {"startTime", {0,0,1}}
        }
        });
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
    // 这里只用空 notes 数组，后面第二步再填
    // 完整的judgeLine结构
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


    // 写到文件
    std::ofstream ofs(outJson_);
    if (!ofs) return false;
    ofs << j.dump(4);
    return true;
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
