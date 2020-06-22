#include "AudioDecoder.h"

namespace duobei::audio {

int AudioDecoder::Decode(uint16_t* buf, uint32_t size, bool raw, uint32_t timestamp) {
    if (raw) {
        play_internal.Play((void *)buf, size, timestamp);
    }
    return 0;
}

bool AudioDecoder::Init(const std::string& stream_id) {
    return play_internal.Init(stream_id);
}

bool AudioDecoder::Destroy() {
    return play_internal.Destroy();
}

}
