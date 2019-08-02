#include "RingBuffer.h"


RingBuffer::RingBuffer(const std::string& n, int c)
    : size_(c), name(n) {
    buffer_ = rbuf_create(size_);
}

RingBuffer::~RingBuffer() {
    clean();
    rbuf_destroy(buffer_);
}

int RingBuffer::write(void * buf, int32_t length) {
    return write(static_cast<uint8_t *>(buf), length);
}

int RingBuffer::write(uint8_t* buf, int32_t length) {
	return rbuf_write(buffer_, buf, length);
}

int RingBuffer::clean() {
    rbuf_clear(buffer_);
    return 0;
}

int RingBuffer::read(uint8_t *buf, int32_t length) {
    return rbuf_read(buffer_, buf, length);
}

int RingBuffer::size() {
    return rbuf_used(buffer_);
}

int MediaBuffer::append(void* buf, size_t size) {
    if (size == 0) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    if (used_ + size > length_) {
        used_ += echoCancelDelayLen_ - length_;
        memmove(data, data + echoCancelDelayLen_, used_);
    }

    memmove(data + used_, buf, size);
    used_ += size;
    // printf("used:%d length:%d size=%d\n", used_, length_, size);
    return 0;
}

void MediaBuffer::reset() {
    std::lock_guard<std::mutex> lock(mtx_);
    used_ = 0;
}

char* MediaBuffer::getData(int size, int delayLen) {
    std::lock_guard<std::mutex> lock(mtx_);
    echoCancelDelayLen_ = delayLen;
    int available = used_;
    int outdated = available - delayLen;
    // printf("used:%d length:%d outdated=%d delayLen=%d\n", used_, length_, outdated, delayLen);
    if (outdated < 0 || size > delayLen) {
        return nullptr;
    }
    return &data[outdated];
}