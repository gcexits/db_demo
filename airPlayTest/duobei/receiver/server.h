#include "raop.h"
#include "logger.h"
#include "stream.h"
#include "dnssd.h"
#include "airplay.h"

#include <string>

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <unistd.h>
#elif WIN32
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

#include "parser/PacketParser.h"
#include "util/Time.h"

namespace duobei::server {

class AirPlayServer{
    raop_t *m_pRaop = nullptr;
    raop_callbacks_t m_pRaop_callback{};
    uint16_t raop_port = 0;

    airplay_t *m_pAirplay = nullptr;
    airplay_callbacks_t m_pAirplay_callback{};
    uint16_t airplay_port = 0;

    dnssd_t *m_pDnsSd = nullptr;

    char hwaddr[6] = {};
    int error_code = 0;

    static void video_get_play_info(void* cls, double* duration, double* position, double* rate);

    bool getMacAddress(char macstr[6]);
    void getHostName(std::string& hostname);

    std::string hostname;

    // todo: save last data, airplay在画面静止一段事件后是不回传输数据，需要开启一个保活线程
    struct AirplayData {
        uint8_t *data;
        int len = 0;
        uint64_t pts;

        void fillBuffer(uint8_t *data_, int len_, uint64_t pts_) {
            if (len == 0) {
                data = new uint8_t[len_];
            }
            if (len < len_) {
                delete [] data;
                data = new uint8_t[len_];
            }
            memcpy(data, data_, len_);
            len = len_;
            pts = pts_;
        }

        void resert() {
            delete [] data;
            len = 0;
            pts = 0;
        }
    };
    bool running;
    std::thread keepliving_;
    void keepLiveLoop();
public:
    int keep_count = 0;
    AirplayData airplayData;
    std::mutex keeplock_;
    bool connectRunning;
    parse_::PacketParser *parser = nullptr;
    explicit AirPlayServer();
    void setParser(parse_::PacketParser &p);
    int initServer();
    int startServer();
    int publishServer();
    void stopServer();
};
}