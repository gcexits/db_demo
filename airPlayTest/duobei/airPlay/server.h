#include "raop.h"

#include <string>

#include <sys/sysctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>

namespace duobei::server {
class AirPlayServer{
    #if 0
    raop_t *m_client = nullptr;
    raop_callbacks_t m_raop_callback{};
    uint16_t raop_port = 0;

    static void video_process(void *cls, h264_decode_struct *data);
    static void audio_process(void *cls, pcm_data_struct *data);

    bool getAddress(std::string& addr);
    void getHostName(std::string& hostname);

public:
    AirPlayServer();
    bool initServer();
    int startServer();
    bool publishServer();
    void stopServer();
    #endif
};
}