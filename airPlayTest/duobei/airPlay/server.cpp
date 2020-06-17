#include "server.h"

#include <unistd.h>

namespace duobei::server {
#if 0
void AirPlayServer::video_process(void* cls, h264_decode_struct* data) {
    AirPlayServer *airPlayServer = static_cast<AirPlayServer *>(cls);
    if (!airPlayServer || data->data_len <= 0) {
        return;
    }
    printf("video data\n");
}

void AirPlayServer::audio_process(void* cls, pcm_data_struct* data) {
    AirPlayServer *airPlayServer = static_cast<AirPlayServer *>(cls);
    if (!airPlayServer || data->data_len <= 0) {
        return;
    }
    printf("audio data\n");
}

bool AirPlayServer::getAddress(std::string &addr) {
#ifdef __APPLE__
    uint32_t mib[6] = {CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST};
    size_t len;
    uint8_t *ptr;
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;

    if ((mib[5] = if_nametoindex("en0")) == 0) {
        return false;
    }

    if (sysctl(reinterpret_cast<int *>(mib), 6, nullptr, &len, nullptr, 0) < 0) {
        return false;
    }

    std::shared_ptr<void> buffer;

    if (sysctl(reinterpret_cast<int *>(mib), 6, buffer.get(), &len, nullptr, 0) < 0) {
        return false;
    }

    ifm = (struct if_msghdr *)buffer.get();
    sdl = (struct sockaddr_dl *)(ifm + 1);
    ptr = (uint8_t *)LLADDR(sdl);

    len = sprintf(&addr[0], "%02x:%02x:%02x:%02x:%02x:%02x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));
    addr.resize(len);
#else
#endif
    return true;
}

void AirPlayServer::getHostName(std::string &hostname) {
#ifdef __APPLE__
    gethostname(const_cast<char *>(hostname.c_str()), 512);
#else
#endif
}

AirPlayServer::AirPlayServer() {
    m_raop_callback.cls = this;
    m_raop_callback.video_process = video_process;
    m_raop_callback.audio_process = audio_process;
}

bool AirPlayServer::initServer() {
    m_client = raop_init(10, &m_raop_callback);
    if (!m_client) {
        return false;
    }
    raop_set_log_level(m_client, RAOP_LOG_DEBUG);
    raop_set_log_callback(m_client, nullptr, this);
    return true;
}

int AirPlayServer::startServer() {
    int ret = raop_start(m_client, &raop_port);
    if (ret < 0) {
        return ret;
    }
    raop_set_port(m_client, raop_port);
    return ret;
}

bool AirPlayServer::publishServer() {
    std::string hostname;
    getHostName(hostname);
    std::string addr;
    if (!getAddress(addr)) {
        return false;
    }
    return true;
}

void AirPlayServer::stopServer() {
    if (m_client) {
        raop_destroy(m_client);
        m_client = nullptr;
    }
}
#endif
}