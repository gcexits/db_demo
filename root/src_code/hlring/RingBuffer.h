//
// Created by Johnny on 7/25/17.
//
#pragma once


extern "C" {
	#include "rbuf.h"
}

#include <string>
#include <mutex>
#include <memory>

class RingBuffer {
    rbuf_t *buffer_;
    int32_t size_;

public:
	using Ptr = std::unique_ptr<RingBuffer>;
    const std::string name;
    RingBuffer(const std::string& name, int size);
    ~RingBuffer();
    static Ptr New(const std::string& name, int size) {
    	return std::make_unique<RingBuffer>(name, size);
    }

	int write(void* buf, int32_t length);
	int write(uint8_t* buf, int32_t length);
    int clean();
    int read(uint8_t* buf, int32_t length);
    int size();
};


class MediaBuffer {
    enum { length_ = 6400 };
    int used_ = 0;
    char data[length_];
    int echoCancelDelayLen_ = 0;
    std::mutex mtx_;
public:
    int append(void* buf, size_t size);
    void reset();
    char* getData(int size, int delayLen);
};
