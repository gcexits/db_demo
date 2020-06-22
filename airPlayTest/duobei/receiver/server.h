#include "raop.h"
#include "logger.h"
#include "stream.h"
#include "dnssd.h"
#include "airplay.h"

#include <string>

#import <sys/sysctl.h>
#import <sys/socket.h>
#import <net/if.h>
#import <net/if_dl.h>

#include <parser/PacketParser.h>

namespace duobei::server {

class AirPlayServer{
    parse_::PacketParser *parser = nullptr;

    raop_t *m_pRaop = nullptr;
    raop_callbacks_t m_pRaop_callback{};
    uint16_t raop_port = 0;

    airplay_t *m_pAirplay = nullptr;
    airplay_callbacks_t m_pAirplay_callback{};
    uint16_t airplay_port = 0;

    dnssd_t *m_pDnsSd = nullptr;

    char hwaddr[6] = {};
    int error_code = 0;

    static void video_process(void *cls, h264_decode_struct *data, const char* remoteName, const char* remoteDeviceId);
    static void audio_process(void *cls, pcm_data_struct *data, const char* remoteName, const char* remoteDeviceId);

    static void video_get_play_info(void* cls, double* duration, double* position, double* rate);

    bool getMacAddress(char macstr[6]);
    void getHostName(std::string& hostname);

public:
    std::string hostname;
    explicit AirPlayServer();
    void setParser(parse_::PacketParser &p);
    int initServer();
    int startServer();
    int publishServer();
    void stopServer();
};
}