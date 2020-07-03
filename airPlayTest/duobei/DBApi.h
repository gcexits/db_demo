#pragma once

#include "parser/PacketParser.h"
#include "util/Callback.h"
#include "receiver/server.h"
#include "util/DBLog.h"

namespace duobei {

class DBApi {
    DBApi() = default;

    static DBApi *instance() {
        static DBApi api_;
        return &api_;
    }

    parse_::PacketParser parser;
    server::AirPlayServer airPlayServer;
#ifndef __APPLE__
    int ShellExecuteExOpen(std::wstring &exe_);
    int GetProcessIDByName(std::wstring &name_);
    int CheckProcess(std::wstring &path);
#endif

public:
    static DBApi *getApi() {
        return instance();
    }

    int startApi(std::string &uid, std::wstring &path, int w, int h);
    int stopApi();
};
}