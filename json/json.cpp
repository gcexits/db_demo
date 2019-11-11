#include <json/document.h>
#include <json/rapidjson.h>
#include <json/stringbuffer.h>
#include <json/writer.h>
#include <json/istreamwrapper.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <list>
#include <map>
#include <vector>
#include <queue>
#include <unordered_map>

#include "RapidJsonWrapper.hpp"

namespace json = rapidjson;

struct Common {
    mutable std::string url;
    std::string sessionId;
    std::string systemId;
    std::string roomId;
    int role;
    int live;
    std::string courseType;
    std::string os;
    std::string platform;
    std::string client;
    std::string clientVer;
    int64_t startTime;
    void AddString(json::Document &document, std::string key, std::string value) const {
        json::Document::AllocatorType &allocator = document.GetAllocator();
        json::Value Key;
        Key.SetString(key.c_str(), allocator);
        json::Value Value;
        Value.SetString(value.c_str(), allocator);
        document.AddMember(Key, Value, allocator);
    }
    void AddInt(json::Document &document, std::string key, int value) const {
        json::Document::AllocatorType &allocator = document.GetAllocator();
        json::Value Key;
        Key.SetString(key.c_str(), allocator);
        json::Value Value;
        Value.SetInt(value);
        document.AddMember(Key, Value, allocator);
    }
    void AddInt64(json::Document &document, std::string key, int value) const {
        json::Document::AllocatorType &allocator = document.GetAllocator();
        json::Value Key;
        Key.SetString(key.c_str(), allocator);
        json::Value Value;
        Value.SetInt64(value);
        document.AddMember(Key, Value, allocator);
    }
    void AddJsonString(json::Document::AllocatorType &allocator, json::Value &json, std::string key, std::string value) const {
        json::Value Key;
        Key.SetString(key.c_str(), allocator);
        json::Value Value;
        Value.SetString(value.c_str(), allocator);
        json.AddMember(Key, Value, allocator);
    }
    void AddJsonBool(json::Document::AllocatorType &allocator, json::Value &json, std::string key, bool value) const {
        json::Value Key;
        Key.SetString(key.c_str(), allocator);
        json::Value Value;
        Value.SetBool(value);
        json.AddMember(Key, Value, allocator);
    }
    void AddJsonArray(json::Document::AllocatorType &allocator, json::Value &json, std::vector<int> vec, std::string key) const {
        if (!vec.empty()) {
            json::Value Key;
            Key.SetString(key.c_str(), allocator);
            json::Value LRValue(json::kArrayType);
            for (auto &l : vec) {
                LRValue.PushBack(l, allocator);
            }
            json.AddMember(Key, LRValue, allocator);
        }
    }
    virtual json::Document dump() const {
        json::Document doc(json::kObjectType);

        AddString(doc, "sessionId", sessionId);
        AddString(doc, "systemId", systemId);
        AddString(doc, "roomId", roomId);
        AddInt(doc, "role", role);
        AddInt(doc, "live", live);
        AddString(doc, "courseType", courseType);
        AddString(doc, "os", os);
        AddString(doc, "platform", platform);
        AddString(doc, "client", client);
        AddString(doc, "clientVer", clientVer);
        AddString(doc, "platform", platform);
        AddInt64(doc, "startTime", startTime);

        return doc;
    }
    void fillData() const {
        url = "www.baidu.com";
    }
};

struct Connect : Common {
    std::string type;
    struct {
        std::string reason;
        bool reconn;
        std::string serverType;
        std::string serverAddr;
        std::string protocol;
        std::string clientNetwork;
    } info;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddString(doc, "type", "connect");
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value infoValue(json::kObjectType);
        {
            AddJsonString(allocator, infoValue, "reason", info.reason);
            AddJsonBool(allocator, infoValue, "reconn", info.reconn);
            AddJsonString(allocator, infoValue, "serverType", info.serverType);
            AddJsonString(allocator, infoValue, "serverAddr", info.serverAddr);
            AddJsonString(allocator, infoValue, "protocol", info.protocol);
            AddJsonString(allocator, infoValue, "clientNetwork", info.clientNetwork);
        }
        json::Value infoKey("info");
        doc.AddMember(infoKey, infoValue, allocator);

        json::StringBuffer s;
        json::Writer<json::StringBuffer> writer(s);
        doc.Accept(writer);

        json::Document parseDocument;
        parseDocument.Parse(s.GetString());
        //        if (parseDocument.HasMember("url")) {
        //            std::cout << << std::endl;
        //        }

        return doc;
    }
};

struct VideoResolution : Common {
    std::string type;
    struct {
        std::string value;
        std::string direction;
        std::string streamUid;
    } info;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddString(doc, "type", "videoresolution");
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value infoValue(json::kObjectType);
        {
            AddJsonString(allocator, infoValue, "value", info.value);
            AddJsonString(allocator, infoValue, "direction", info.direction);
            AddJsonString(allocator, infoValue, "streamUid", info.streamUid);
        }
        json::Value infoKey("info");
        doc.AddMember(infoKey, infoValue, allocator);

        return doc;
    }
};

struct MediaParam : Common {
    std::string type;
    struct {
        std::string type;
        std::string value;
    } info;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddString(doc, "type", "videoresolution");
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value infoValue(json::kObjectType);
        {
            AddJsonString(allocator, infoValue, "type", info.type);
            AddJsonString(allocator, infoValue, "value", info.value);
        }
        json::Value infoKey("info");
        doc.AddMember(infoKey, infoValue, allocator);

        return doc;
    }
};

struct PingHistory : Common {
    std::string type;
    int step;
    int64_t fromTimestamp;
    struct {
        std::vector<int> LR;
        std::vector<int> FR;
        std::vector<int> SR;
        std::vector<int> entryRTT;
    } range;
    struct {
        std::string serverType;
    } info;
    json::Document dump() const override {
        fillData();
        auto doc = Common::dump();

        AddString(doc, "type", "pinghistory");
        AddInt(doc, "step", 9000);
        AddInt64(doc, "fromTimestamp", fromTimestamp);
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value rangValue(json::kObjectType);
        {
            AddJsonArray(allocator, rangValue, range.LR, "LR");
            AddJsonArray(allocator, rangValue, range.FR, "FR");
            AddJsonArray(allocator, rangValue, range.SR, "SR");
            AddJsonArray(allocator, rangValue, range.entryRTT, "entryRTT");
        }
        json::Value rangKey("range");
        doc.AddMember(rangKey, rangValue, allocator);

        json::Value infoValue(json::kObjectType);
        {
            AddJsonString(allocator, infoValue, "serverType", info.serverType);
        }
        json::Value infoKey("info");
        doc.AddMember(infoKey, infoValue, allocator);

        return doc;
    }
};

// note: 音视频上下行
struct MediaUpLinkRate : Common {
    std::string type;
    int step;
    struct {
        std::vector<int> video;
        std::vector<int> audio;
    } range;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddString(doc, "type", "mediauplinkrate");
        AddInt(doc, "step", step);
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value rangValue(json::kObjectType);
        {
            json::Value videoUpLinkValue(json::kArrayType);
            for (auto &v : range.video) {
                videoUpLinkValue.PushBack(v, allocator);
            }
            rangValue.AddMember("video", videoUpLinkValue, allocator);

            json::Value audioUpLinkValue(json::kArrayType);
            for (auto &a : range.audio) {
                audioUpLinkValue.PushBack(a, allocator);
            }
            rangValue.AddMember("audio", audioUpLinkValue, allocator);
        }
        json::Value _userIdKey("range");
        doc.AddMember(_userIdKey, rangValue, allocator);

        return doc;
    }
};

struct MediaDownLinkRate : Common {
    std::string type;
    int step;
    int64_t fromTimestamp;
    struct {
        std::vector<int> video;
        std::vector<int> audio;
    } range;
    struct {
        std::string sid;
    } info;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddString(doc, "type", "mediadownlinkrate");
        AddInt(doc, "step", step);
        AddInt64(doc, "fromTimestamp", fromTimestamp);
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value rangValue(json::kObjectType);
        {
            json::Value videoDownLinkValue(json::kArrayType);
            for (auto &v : range.video) {
                videoDownLinkValue.PushBack(v, allocator);
            }
            rangValue.AddMember("video", videoDownLinkValue, allocator);

            json::Value audioDownLinkValue(json::kArrayType);
            for (auto &a : range.audio) {
                audioDownLinkValue.PushBack(a, allocator);
            }
            rangValue.AddMember("audio", audioDownLinkValue, allocator);
        }
        json::Value rangdKey("range");
        doc.AddMember(rangdKey, rangValue, allocator);

        json::Value infoValue(json::kObjectType);
        {
            AddJsonString(allocator, infoValue, "sid", info.sid);
        }
        json::Value infoKey("info");
        doc.AddMember(infoKey, infoValue, allocator);

        return doc;
    }
};

void dump(json::Document &doc) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    doc.Accept(writer);
    std::cout << s.GetString() << std::endl;
}

struct AVInfo {
    std::string uid;
    struct MediaParam {
        double down = 0;
        std::string url;
        double fps = 0;
    };
    MediaParam audio;
    MediaParam video;
};
using AVInfos = std::vector<AVInfo>;

void Check(std::list<std::pair<std::string, int>> &List, std::pair<std::string, int> pair) {
    if (List.empty()) {
        List.emplace_back(pair);
    } else {
        for (auto it = List.begin(); it != List.end(); ++it){
            if (it->first == pair.first) {
                it = List.erase(it);
                break;
            }
        }
        List.emplace_back(pair);
    }
}

int main() {
//    duobei::JsonBuilder jb;
//    {
//        auto object = jb.object();
//        object["room"] = "jz";
//    }
//    std::cout << jb.toString() << std::endl;
//    std::queue<int> q;
//    q.push(1);
//    q.push(2);
//    q.push(3);
//    q.push(4);
//    q.pop();
//    q.pop();
//    q.pop();
//    while (!q.empty()) {
//        std::cout << ' ' << q.front() << std::endl;
//        break;
//    }
//    std::unordered_map<std::string, int> map;
//    map.emplace("1", 1);
//    map.erase("2");
//    for (auto &item : map) {
//        std::cout << item.first << std::endl;
//    }
//    using Pair = std::pair<std::string, int>;
//    std::list<Pair> OnLineList;
//    std::list<Pair> OffLineList;
//    Pair pair;
//    pair = std::make_pair("1", 1);
//    Check(OnLineList, pair);
//
//    Pair pair_1;
//    pair_1 = std::make_pair("3", 2);
//    Check(OnLineList, pair_1);
//
//    Pair pair_2;
//    pair_2 = std::make_pair("1", 3);
//    Check(OnLineList, pair_2);
//
//    Pair pair_3;
//    pair_3 = std::make_pair("1", 6);
//    Check(OffLineList, pair_3);
//
//
//    for (auto it = OnLineList.begin(); it != OnLineList.end(); it++) {
//        for (auto itof = OffLineList.begin(); itof != OffLineList.end(); itof++) {
//            if (it->first == itof->first) {
//                if (it->second < itof->second) {
//                    std::cout << "offline" << std::endl;
//                } else {
//                    std::cout << "online" << std::endl;
//                }
//            } else {
//                std::cout << "online" << std::endl;
//            }
//        }
//    }
//    return 0;
//    std::string startTime = "123213414124112412";
//    int64_t time = startTime.empty() ? 0 : std::stoll(startTime);
//    std::cout << time << std::endl;
//
//    AVInfos avInfos;
//    for (int i = 0; i < 3; i++) {
//        struct AVInfo avinfo;
//        avinfo.audio.down = i;
//        avinfo.video.down = i;
//        avinfo.video.fps = i;
//        avInfos.push_back(avinfo);
//    }
//    for (auto & av : avInfos) {
//        std::cout << av.audio.down << " " << av.video.down << " " << av.video.fps << std::endl;
//    }
//    return 0;
#if 0
    using Pair = std::pair<std::string, std::string>;
    Connect connect;
    auto doc_1 = connect.dump();
    dump(doc_1);
    std::list<Pair> list;
    Pair pair;
    pair = std::make_pair("1", "");
    list.push_back(pair);
    for (auto & l : list) {
        if (!l.second.empty()) {
            std::cout << l.first << " " << l.second << std::endl;
        }
    }

    VideoResolution videoResolution;
    auto doc_2 = videoResolution.dump();
    dump(doc_2);

    MediaParam mediaParam;
    auto doc_3 = mediaParam.dump();
    dump(doc_3);

    MediaUpLinkRate mediaUpLinkRate;
    mediaUpLinkRate.range.video.push_back(1);
    mediaUpLinkRate.range.video.push_back(2);
    mediaUpLinkRate.range.video.push_back(3);

    mediaUpLinkRate.range.audio.push_back(4);
    mediaUpLinkRate.range.audio.push_back(5);
    mediaUpLinkRate.range.audio.push_back(6);
    auto doc_4 = mediaUpLinkRate.dump();
    dump(doc_4);
#else
//    MediaDownLinkRate mediaDownLinkRate;
//    mediaDownLinkRate.range.video.push_back(7);
//    mediaDownLinkRate.range.video.push_back(8);
//    mediaDownLinkRate.range.video.push_back(9);
//
//    mediaDownLinkRate.range.audio.push_back(10);
//    mediaDownLinkRate.range.audio.push_back(11);
//    mediaDownLinkRate.range.audio.push_back(12);
//    auto doc_5 = mediaDownLinkRate.dump();
//    dump(doc_5);
//    PingHistory pingHistory;
//    pingHistory.range.entryRTT.push_back(1);
//    pingHistory.range.entryRTT.push_back(2);
//    pingHistory.range.entryRTT.push_back(3);
//    auto doc_6 = pingHistory.dump();
//    dump(doc_6);
#endif
    // https://rapidjson.org/md_doc_stream.html
    using namespace rapidjson;

    std::fstream ifs("/Users/guochao/dby-client-tool.json", std::ios::binary | std::ios::in);
    if ( !ifs.is_open() )
    {
        std::cerr << "Could not open file for reading!\n";
        return EXIT_FAILURE;
    }

    IStreamWrapper isw { ifs };

    Document doc {};
    doc.ParseStream( isw );

    StringBuffer buffer {};
    Writer<StringBuffer> writer { buffer };
    doc.Accept( writer );

    if ( doc.HasParseError() )
    {
        std::cout << "Error  : " << doc.GetParseError()  << '\n'
                  << "Offset : " << doc.GetErrorOffset() << '\n';
        return EXIT_FAILURE;
    }

    const std::string jsonStr { buffer.GetString() };

    std::cout << jsonStr << '\n';



    return 0;
}