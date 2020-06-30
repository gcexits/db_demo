#include "DBApi.h"

namespace duobei {

DBApi::DBApi() {
    std::string uid = "123";
    parser.Init(uid);
    airPlayServer.setParser(parser);
}

int DBApi::startApi() {
    int ret = 0;
    if ((ret = airPlayServer.initServer()) < 0) {
        WriteErrorLog("airPlayServer.initServer error : %d", ret);
        return ret;
    }
    if ((ret = airPlayServer.startServer()) < 0) {
        WriteErrorLog("airPlayServer.startServer error : %d", ret);
        return ret;
    }
    if ((ret = airPlayServer.publishServer()) < 0) {
        WriteErrorLog("airPlayServer.publishServer error : %d", ret);
        return ret;
    }
    return ret;
}

int DBApi::stopApi() {
    airPlayServer.stopServer();
    return 0;
}

}