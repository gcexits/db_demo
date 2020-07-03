#include "raop.h"
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
#else
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#pragma comment(lib, "iphlpapi.lib")
#endif

#include "parser/PacketParser.h"
#include "util/Time.h"
#include "util/Callback.h"
#include "common/StateCode.h"

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

    std::string hostname = "gc-windows-demo";
    bool connect_ = false;

public:
    parse_::PacketParser *parser = nullptr;
    explicit AirPlayServer();
    void setParser(parse_::PacketParser &p);
    int initServer();
    int startServer();
    int publishServer();
    void stopServer();
    void setConnect(bool state);
    bool invalid();
};

}