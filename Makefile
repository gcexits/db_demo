CC = g++

#SOURCE = 1_audio_mix.cpp
SOURCE = 2_audio_decode.cpp
TARGET = mix.out

LIBDIR = -L/usr/local/Cellar/ffmpeg/4.1.1/lib

INCLDIR = -I/usr/local/Cellar/ffmpeg/4.1.1/include

OPTIONAL = -O2 -Wall -g -std=c++14
LIB_FFMPEG = -lavcodec -lavformat -lswresample

all:
	$(CC) $(SOURCE) -o $(TARGET) $(LIBDIR) $(INCLDIR) $(OPTIONAL) $(LIB_FFMPEG)
	ulimit -c unlimited

clean:
	rm -rf *.out *.pcm *.dSYM
