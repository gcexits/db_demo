#pragma once

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "../src_code/display/SDLPlayer.h"

struct Argument {
    SDLPlayer player;
    struct Param {
        std::string playerUrl;
        std::string senderUrl;
        std::string recvUrl;
        std::string filterUrl;
        std::string flvUrl;
        int launch;
        std::string profile =  "/Users/guochao/DBY_code/ff_test/Configuration_profile.json";
        struct Device {
            int index;
            std::string name;
            std::string format;
        };
        Device device;
        std::string h264;
        std::string yuv;
        std::string pcm;
        struct Mix {
            std::string src_audio_1;
            std::string src_audio_2;
            std::string dst_pcm;
        };
        Mix mix;
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
        assert(fhandle.is_open());
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
        if (p.HasMember("filterUrl") && p["filterUrl"].IsString()) {
            param.filterUrl = p["filterUrl"].GetString();
        }
        if (p.HasMember("senderUrl") && p["senderUrl"].IsString()) {
            param.senderUrl = p["senderUrl"].GetString();
        }
        if (p.HasMember("recvUrl") && p["recvUrl"].IsString()) {
            param.recvUrl = p["recvUrl"].GetString();
        }
        if (p.HasMember("flvUrl") && p["flvUrl"].IsString()) {
            param.flvUrl = p["flvUrl"].GetString();
        }
        if (p.HasMember("capture") && p["capture"].IsObject()) {
            auto& device = p["capture"];
            param.device.index = device["index"].GetInt();
            param.device.name = device["name"].GetString();
            param.device.format = device["format"].GetString();
        }
        if (p.HasMember("local") && p["local"].IsObject()) {
            auto& source = p["local"];
            param.h264 = source["h264"].GetString();
            param.pcm = source["pcm"].GetString();
            param.yuv = source["yuv"].GetString();
        }
        if (p.HasMember("mix") && p["mix"].IsObject()) {
            auto& mix = p["mix"];
            param.mix.src_audio_1 = mix["src_audio_1"].GetString();
            param.mix.src_audio_2 = mix["src_audio_2"].GetString();
            param.mix.dst_pcm = mix["dst_pcm"].GetString();
        }
    }
};