#pragma once

#include "json.h"

#include "../src_code/display/SDLPlayer.h"

struct Argument {
    SDLPlayer player;
    struct Param {
        std::string playerUrl;
        std::string senderUrl;
        int launch;
        std::string profile =  "/Users/guochao/DBY_code/ff_test/Configuration_profile.json";
        struct Device {
            int index;
            std::string name;
            std::string format;
        };
        Device device;
    };
    Param param;

    void RegisterPlayer() {
        using namespace std::placeholders;
        AVRegister::setinitVideoPlayer(std::bind(&SDLPlayer::openVideo, &player, _1, _2));
        AVRegister::setinitPcmPlayer(std::bind(&SDLPlayer::openAudio, &player, _1, _2));

        AVRegister::setdestroyVideoPlayer(std::bind(&SDLPlayer::closeVideo, &player, _1));
        AVRegister::setdestroyPcmPlayer(std::bind(&SDLPlayer::closeAudio, &player, _1));
    }

    void LoadProfile() {
        std::ifstream fhandle(param.profile, std::ios::binary | std::ios::in);
        if (!fhandle.is_open()) {
            return;
        }
        rapidjson::IStreamWrapper is(fhandle);
        rapidjson::Document doc;
        doc.ParseStream(is);
        assert(doc.HasMember("profile"));
        auto& p = doc["profile"];
        if (p.HasMember("launch") && p["launch"].IsInt()) {
            param.launch = p["launch"].GetInt();
        }
        if (p.HasMember("playerUrl") && p["playerUrl"].IsString()) {
            param.playerUrl = p["playerUrl"].GetString();
        }
        if (p.HasMember("senderUrl") && p["senderUrl"].IsString()) {
            param.senderUrl = p["senderUrl"].GetString();
        }
        if (p.HasMember("capture") && p["capture"].IsObject()) {
            auto& device = p["capture"];
            if (device.HasMember("index") && device["index"].IsInt()) {
                param.device.index = device["index"].GetInt();
            }
            if (device.HasMember("name") && device["name"].IsString()) {
                param.device.name = device["name"].GetString();
            }
            if (device.HasMember("format") && device["format"].IsString()) {
                param.device.format = device["format"].GetString();
            }
        }
    }
};