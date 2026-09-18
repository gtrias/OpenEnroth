cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(MEDIA_SOURCES
        AudioBaseDataSource.cpp
        AudioBufferDataSource.cpp
        FFmpegBlobIoContext.cpp
        FFmpegLogProxy.cpp
        FFmpegLogSource.cpp
        MediaPlayer.cpp)

set(MEDIA_HEADERS
        AudioBaseDataSource.h
        AudioBufferDataSource.h
        AudioDataSource.h
        AudioSample.h
        AudioTrack.h
        FFmpegBlobIoContext.h
        FFmpegLogProxy.h
        FFmpegLogSource.h
        MediaPlayer.h
        Movie.h
        VideoDataSource.h)

add_library(media STATIC ${MEDIA_SOURCES} ${MEDIA_HEADERS})
target_link_libraries(media PRIVATE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale FFmpeg::swresample)

if(OE_BUILD_PLATFORM STREQUAL "vita")
    # avutil again at the end: swscale references av_tree_* and is linked after avutil otherwise;
    # dl is vita-rss-libdl; mp3lame is a dep of the ffmpeg mp3 encoder.
    target_link_libraries(media PRIVATE avutil dl mp3lame mpg123)
endif()
target_link_libraries(media PUBLIC media_audio library_logger library_vid utility application)

target_check_style(media)

add_subdirectory(Audio)
