
#include "PacketQueue.h"
#include <iostream>

PacketQueue::PacketQueue() {
    nb_packets = 0;
    size = 0;

    mutex = SDL_CreateMutex();
    cond = SDL_CreateCond();
}

bool PacketQueue::enQueue(const AVPacket *packet) {
    AVPacket *pkt = av_packet_alloc();
    if (av_packet_ref(pkt, packet) < 0)
        return false;

    SDL_LockMutex(mutex);
    queue.push(*pkt);

    size += pkt->size;
    nb_packets++;

    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);
    return true;
}

bool PacketQueue::deQueue(AVPacket *packet, bool block) {
    bool ret = false;

    SDL_LockMutex(mutex);
    while (true) {
        if (!queue.empty()) {
            av_packet_move_ref(packet, &queue.front());
            AVPacket pkt = queue.front();

            queue.pop();
            nb_packets--;
            size -= packet->size;

            ret = true;
            break;
        } else if (!block) {
            ret = false;
            break;
        } else {
            SDL_CondWait(cond, mutex);
        }
    }
    SDL_UnlockMutex(mutex);
    return ret;
}