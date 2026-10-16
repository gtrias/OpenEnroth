#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "Library/Vid/VidReader.h"

struct AVIOContext;
class InputStream;

/** Adapts a VID entry stream to FFmpeg's AVIO interface. */
class FFmpegVidIoContext {
 public:
    FFmpegVidIoContext() = default;
    ~FFmpegVidIoContext();

    /**
     * Configures FFmpeg callbacks to read one entry from a VID.
     *
     * @param reader                    VID reader containing the entry.
     * @param entry                     Entry name inside the VID.
     */
    void reset(VidReader &reader, std::string_view entry);

    AVIOContext *avioContext() const {
        return _context;
    }

 private:
    friend int ffVidRead(void *opaque, uint8_t *buffer, int size);
    friend int64_t ffVidSeek(void *opaque, int64_t offset, int whence);

    void createAvioContext();
    void destroyAvioContext();

    VidReader *_reader = nullptr;
    std::string _entry;
    std::unique_ptr<InputStream> _stream;
    size_t _size = 0;
    size_t _position = 0;
    AVIOContext *_context = nullptr;
};
