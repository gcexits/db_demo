#include <json/document.h>
#include <json/rapidjson.h>
#include <json/stringbuffer.h>
#include <json/writer.h>

#include <iostream>
#include <vector>

namespace json = rapidjson;

struct Common {
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
};

struct Connect: Common {
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
        if (parseDocument.HasParseError()) {
            exit(-1);
        }

        return doc;
    }
};

struct VideoResolution: Common {
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

struct MediaParam: Common {
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

// note: 音视频上下行
struct MediaUpLinkRate: Common {
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

struct MediaDownLinkRate: Common {
    std::string type;
    int step;
    int64_t fromTimestamp;
    std::string streamUid;
    struct {
        std::vector<int> video;
        std::vector<int> audio;
    } range;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddString(doc, "type", "mediadownlinkrate");
        AddInt(doc, "step", step);
        AddInt64(doc, "fromTimestamp", fromTimestamp);
        AddString(doc, "streamUid", streamUid);
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value userValue(json::kObjectType);
        {
            json::Value videoDownLinkValue(json::kArrayType);
            for (auto &v : range.video) {
                videoDownLinkValue.PushBack(v, allocator);
            }
            userValue.AddMember("video", videoDownLinkValue, allocator);

            json::Value audioDownLinkValue(json::kArrayType);
            for (auto &a : range.audio) {
                audioDownLinkValue.PushBack(a, allocator);
            }
            userValue.AddMember("audio", audioDownLinkValue, allocator);
        }
        json::Value _userIdKey("range");
        doc.AddMember(_userIdKey, userValue, allocator);

        return doc;
    }
};

void dump(json::Document &doc) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    doc.Accept(writer);
    std::cout << s.GetString() << std::endl;
}

int main() {
    Connect connect;
    auto doc_1 = connect.dump();
    dump(doc_1);

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

    MediaDownLinkRate mediaDownLinkRate;
    mediaDownLinkRate.range.video.push_back(7);
    mediaDownLinkRate.range.video.push_back(8);
    mediaDownLinkRate.range.video.push_back(9);

    mediaDownLinkRate.range.audio.push_back(10);
    mediaDownLinkRate.range.audio.push_back(11);
    mediaDownLinkRate.range.audio.push_back(12);
    auto doc_5 = mediaDownLinkRate.dump();
    dump(doc_5);

    return 0;
}