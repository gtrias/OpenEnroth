#include "Media/FFmpegVidIoContext.h"

#include <cerrno>
#include <cstdio>
#include <new>
#include <string>

extern "C" {
#include <libavformat/avio.h> // NOLINT: not a C system header.
#include <libavutil/error.h> // NOLINT: not a C system header.
#include <libavutil/mem.h> // NOLINT: not a C system header.
}

#include "Utility/Streams/InputStream.h"

int ffVidRead(void *opaque, uint8_t *buffer, int size) {
    FFmpegVidIoContext *context = static_cast<FFmpegVidIoContext *>(opaque);
    if (size <= 0)
        return size == 0 ? 0 : AVERROR(EINVAL);

    try {
        size_t bytesRead = context->_stream->read(buffer, static_cast<size_t>(size));
        context->_position += bytesRead;
        return bytesRead == 0 ? AVERROR_EOF : static_cast<int>(bytesRead);
    } catch (...) {
        return AVERROR(EIO);
    }
}

int64_t ffVidSeek(void *opaque, int64_t offset, int whence) {
    FFmpegVidIoContext *context = static_cast<FFmpegVidIoContext *>(opaque);
    unsigned int seekFlags = static_cast<unsigned int>(whence);
    if (seekFlags & static_cast<unsigned int>(AVSEEK_SIZE))
        return static_cast<int64_t>(context->_size);

    int64_t base;
    switch (seekFlags & ~static_cast<unsigned int>(AVSEEK_FORCE)) {
    case SEEK_SET:
        base = 0;
        break;
    case SEEK_CUR:
        base = static_cast<int64_t>(context->_position);
        break;
    case SEEK_END:
        base = static_cast<int64_t>(context->_size);
        break;
    default:
        return AVERROR(EINVAL);
    }

    int64_t size = static_cast<int64_t>(context->_size);
    if (offset < -base || offset > size - base)
        return AVERROR(EINVAL);
    int64_t position = base + offset;

    try {
        context->_stream = context->_reader->openEntryStream(context->_entry);
        size_t bytesSkipped = context->_stream->skip(static_cast<size_t>(position));
        if (bytesSkipped != static_cast<size_t>(position))
            return AVERROR(EIO);
        context->_position = static_cast<size_t>(position);
        return position;
    } catch (...) {
        return AVERROR(EIO);
    }
}

FFmpegVidIoContext::~FFmpegVidIoContext() {
    destroyAvioContext();
}

void FFmpegVidIoContext::reset(VidReader &reader, std::string_view entry) {
    destroyAvioContext();
    _reader = &reader;
    _entry = std::string(entry);
    _stream = _reader->openEntryStream(_entry);
    _size = _stream->size();
    _position = 0;
    createAvioContext();
}

void FFmpegVidIoContext::createAvioContext() {
    constexpr int bufferSize = 4096;
    unsigned char *buffer = static_cast<unsigned char *>(av_malloc(bufferSize));
    if (!buffer)
        throw std::bad_alloc();
    _context = avio_alloc_context(buffer, bufferSize, 0, this, &ffVidRead, nullptr, &ffVidSeek);
    if (!_context) {
        av_free(buffer);
        throw std::bad_alloc();
    }
}

void FFmpegVidIoContext::destroyAvioContext() {
    if (_context) {
        av_free(_context->buffer);
        avio_context_free(&_context);
    }
    _stream.reset();
    _reader = nullptr;
    _entry.clear();
    _size = 0;
    _position = 0;
}
