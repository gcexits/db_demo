#pragma once

#include "json.h"

struct Argument {
    SDLPlayer player;
    struct Param {
        std::string url;
        int launch;
        std::string profile =  "/Users/guochao/DBY_code/ff_test/Configuration_profile.json";
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
        if (p.HasMember("url") && p["url"].IsString()) {
            param.url = p["url"].GetString();
        }
    }
};