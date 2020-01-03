#pragma once

#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "RapidJsonWrapper.hpp"
#include "Param.h"

namespace json = rapidjson;

struct Common {
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
        return doc;
    }
};

struct PingHistory : Common {
    int step;
    struct {
        std::vector<int> entryRTT;
    } range;
    struct {
        std::string serverType;
    } info;
    json::Document dump() const override {
        auto doc = Common::dump();

        AddInt(doc, "step", 9000);
        json::Document::AllocatorType &allocator = doc.GetAllocator();

        json::Value rangValue(json::kObjectType);
        {
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

void dump(json::Document &doc);

int main_json(Argument& cmd);