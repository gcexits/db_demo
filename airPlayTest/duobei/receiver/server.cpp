#include "server.h"

namespace duobei::server {

void video_process(void* cls, h264_decode_struct* data, const char* remoteName, const char* remoteDeviceId) {
    AirPlayServer *airPlayServer = static_cast<AirPlayServer *>(cls);
    if (!airPlayServer || data->data_len <= 0 || airPlayServer->invalid()) {
        return;
    }
    airPlayServer->parser->decodeH264Data(data->data, data->data_len, data->frame_type, data->pts);
}

void audio_process(void* cls, pcm_data_struct* data, const char* remoteName, const char* remoteDeviceId) {
    return;
    AirPlayServer *airPlayServer = static_cast<AirPlayServer *>(cls);
    if (!airPlayServer || data->data_len <= 0 || airPlayServer->invalid()) {
        return;
    }
    airPlayServer->parser->dealPcmData(data->data, data->data_len, true, data->pts);
}

void raop_log_callback(void *cls, int level, const char *msg) {
}

void raop_connected(void* cls, const char* remoteName, const char* remoteDeviceId) {
    AirPlayServer *airPlayServer = static_cast<AirPlayServer *>(cls);
    if (!airPlayServer) {
        return;
    }
    airPlayServer->parser->BeginAirPlay();
    airPlayServer->setConnect(true);
    Callback::statusCodeCall(AIRPLAY_CONNECT_OK);
    WriteDebugLog("%s is connected", remoteName, remoteDeviceId);
}

void raop_disconnected(void* cls, const char* remoteName, const char* remoteDeviceId) {
    AirPlayServer *airPlayServer = static_cast<AirPlayServer *>(cls);
    if (!airPlayServer) {
        return;
    }
    airPlayServer->setConnect(false);
    airPlayServer->parser->StopAirPlay();
    Callback::statusCodeCall(AIRPLAY_DISCONNECT);
    WriteDebugLog("%s is disconnected", remoteName, remoteDeviceId);
}

void AirPlayServer::video_get_play_info(void *cls, double *duration, double *position, double *rate) {
}

bool AirPlayServer::getMacAddress(char macstr[6]) {
#ifdef __APPLE__
    uint32_t mib[6] = {CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST};
    size_t len;
    uint8_t *ptr;
    char *buf;
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;

    if ((mib[5] = if_nametoindex("en0")) == 0) {
        return false;
    }

    if (sysctl(reinterpret_cast<int *>(mib), 6, nullptr, &len, nullptr, 0) < 0) {
        return false;
    }

    buf = new char[len];

    if (sysctl(reinterpret_cast<int *>(mib), 6, buf, &len, nullptr, 0) < 0) {
        return false;
    }

    ifm = (struct if_msghdr *)buf;
    sdl = (struct sockaddr_dl *)(ifm + 1);
    ptr = (uint8_t *)LLADDR(sdl);

    for (int i = 0; i < 6; ++i) {
        macstr[i] = *(ptr+i);
    }
    delete [] buf;
#else
    PIP_ADAPTER_INFO pAdapterInfo;
	DWORD AdapterInfoSize;
	DWORD Err;
	AdapterInfoSize = 0;
	Err = GetAdaptersInfo(nullptr, &AdapterInfoSize);
	if ((Err != 0) && (Err != ERROR_BUFFER_OVERFLOW)) {
		return false;
	}
	pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, AdapterInfoSize);
	if (!pAdapterInfo) {
		return false;
	}
	if (GetAdaptersInfo(pAdapterInfo, &AdapterInfoSize) != 0) {
		GlobalFree(pAdapterInfo);
		return false;
	}
	for (int i = 0; i < 6; i++) {
		macstr[i] = pAdapterInfo->Address[i];
	}

	GlobalFree(pAdapterInfo);
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
    m_pRaop_callback.cls = this;
    using namespace std::placeholders;
    m_pRaop_callback.video_process = video_process;
    m_pRaop_callback.audio_process = audio_process;
    m_pRaop_callback.connected = raop_connected;
    m_pRaop_callback.disconnected = raop_disconnected;

    m_pAirplay_callback.cls = this;
    m_pAirplay_callback.video_get_play_info = video_get_play_info;
}

void AirPlayServer::setParser(parse_::PacketParser &p) {
    parser = &p;
}

int AirPlayServer::initServer() {
    m_pRaop = raop_init(10, &m_pRaop_callback);
    if (!m_pRaop) {
        error_code = -1;
        return error_code;
    }
    raop_set_log_level(m_pRaop, RAOP_LOG_WARNING);
    raop_set_log_callback(m_pRaop, nullptr, this);

    m_pAirplay = airplay_init(10, &m_pAirplay_callback, nullptr, &error_code);
    if (!m_pAirplay) {
        return error_code;
    }

    airplay_set_log_level(m_pAirplay, RAOP_LOG_WARNING);
    airplay_set_log_callback(m_pAirplay, nullptr, this);

    getHostName(hostname);
    if (!getMacAddress(hwaddr)) {
        error_code = -1;
        return error_code;
    }

    m_pDnsSd = dnssd_init(&error_code);
    return error_code;
}

int AirPlayServer::startServer() {
    error_code = raop_start(m_pRaop, &raop_port);
    if (error_code < 0) {
        return error_code;
    }
    raop_set_port(m_pRaop, raop_port);

    error_code = airplay_start(m_pAirplay, &airplay_port, hwaddr, 6, nullptr);
    return error_code;
}

int AirPlayServer::publishServer() {
    error_code = dnssd_register_raop(m_pDnsSd, hostname.c_str(), raop_port, hwaddr, 6, 0);
    if (error_code < 0) {
        return error_code;
    }
    error_code = dnssd_register_airplay(m_pDnsSd, hostname.c_str(), airplay_port, hwaddr, 6);
    if (error_code < 0) {
        return error_code;
    }
    WriteDebugLog("%s airPlayServer begin !!!", hostname.c_str());
    connect_ = false;
    return 0;
}

void AirPlayServer::stopServer() {
    if (m_pDnsSd) {
        dnssd_unregister_airplay(m_pDnsSd);
        dnssd_unregister_raop(m_pDnsSd);
        dnssd_destroy(m_pDnsSd);
        m_pDnsSd = nullptr;
    }

    if (m_pRaop) {
        raop_destroy(m_pRaop);
        m_pRaop = nullptr;
    }

    if (m_pAirplay) {
        airplay_destroy(m_pAirplay);
 		airplay_set_log_callback(m_pAirplay, nullptr, this);
        m_pAirplay = nullptr;
    }

    setConnect(false);
    if (parser) {
        parser->StopAirPlay();
    }
}

void AirPlayServer::setConnect(bool state) {
    connect_ = state;
}

bool AirPlayServer::invalid() {
    return !parser || !connect_;
}

}