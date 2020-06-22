#pragma once

#include "parser/PacketParser.h"
#include "util/Callback.h"
#include "receiver/server.h"
#include "util/DBLog.h"

namespace duobei {

class DBApi {
    DBApi();

    static DBApi *instance() {
        static DBApi api_;
        return &api_;
    }

    parse_::PacketParser parser;
    server::AirPlayServer airPlayServer;

public:
    static DBApi *getApi() {
        return instance();
    }

    int startApi();
    int stopApi();
};
}