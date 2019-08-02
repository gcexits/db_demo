#include <iostream>
#include <thread>
#include <csignal>

extern "C" {
#include "vlc/vlc.h"
}

int main() {
    libvlc_instance_t *inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;

    libvlc_time_t length;
    int width;
    int height;
    int wait_time = 5000;

    //libvlc_time_t length;
    sigset_t set;

    signal(SIGCHLD, SIG_DFL);
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    /* Load the VLC engine */
    inst = libvlc_new(0, nullptr);

    //Create a new item
    //Method 1:
    //m = libvlc_media_new_location (inst, "file:///F:\\movie\\cuc_ieschool.flv");
    //Screen Capture
    //m = libvlc_media_new_location (inst, "screen://");
    //Method 2:
    m = libvlc_media_new_path(inst, "/Users/guochao/DBY_code/ff_test/1.flv");

    /* Create a media player playing environement */
    mp = libvlc_media_player_new_from_media(m);

    /* No need to keep the media now */
    libvlc_media_release(m);

    // play the media_player
    libvlc_media_player_play(mp);

    //wait until the tracks are created
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
    length = libvlc_media_player_get_length(mp);
    width = libvlc_video_get_width(mp);
    height = libvlc_video_get_height(mp);
    printf("Stream Duration: %ds\n", length / 1000);
    printf("Resolution: %d x %d\n", width, height);
    //Let it play
    std::this_thread::sleep_for(std::chrono::milliseconds(length - wait_time));

    // Stop playing
    libvlc_media_player_stop(mp);

    // Free the media_player
    libvlc_media_player_release(mp);

    libvlc_release(inst);

    return 0;
}