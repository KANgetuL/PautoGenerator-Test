// PautoGenerator-Test.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "include/json.hpp"
#include <nlohmann/json.hpp>
#include "AudioProcessor.h"

using namespace std;

class Generator{
public:
    //传入用户输入
    //bpm：保留一位小数
    //bgPath：背景图所在路径
    //mp3Path：MP3所在路径
    Generator(float bpm,const string& bgPath,const string& mp3Path);

    //第一步，随机生成8位num，拷贝jpg，转码ogg，输出json
    //成功返回true
    bool processStep1();
    bool processStep2();

    std::string getJsonName() const { return outJson_; }
    std::string getJpgName() const { return outJpg_; }
    std::string getOggName() const { return outOgg_; }


private:
    float bpm_;
    std::string bgPath_,mp3Path_;
    std::string id_;
    std::string outJpg_,outOgg_,outJson_;


    //8位num生成
    std::string generateId();

    //bgPath_ -> id_.jpg
    bool copyBackground();

    //ffmpeg转 mp3 -> id_.ogg
    bool convertSong();

    //template -> 目标json
    bool buildAndWriteJson();

    bool generateNotes();

    nlohmann::json loadJson() const;
    void saveJson(const nlohmann::json& j) const;
};

// TODO: 在此处引用程序需要的其他标头。
