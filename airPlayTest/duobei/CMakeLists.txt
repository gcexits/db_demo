project(client)

set(DECODE_SOURCE_FILES
        ${CLIENT}/DBApi.cpp
        ${CLIENT}/decodec/DecoderSpan.cpp
        ${CLIENT}/decodec/H264Decoder.cpp
        ${CLIENT}/decodec/AudioDecoder.cpp
        ${CLIENT}/decodec/PlayInternal.cpp
        ${CLIENT}/parser/PacketParser.cpp
        ${CLIENT}/receiver/server.cpp
        ${CLIENT}/util/Callback.cpp
        ${CLIENT}/util/DBLog.cpp
        ${CLIENT}/util/Time.cpp
        )

add_library(client_library ${DECODE_SOURCE_FILES})
target_include_directories(client_library PUBLIC ${PROJECT_SOURCE_DIR})