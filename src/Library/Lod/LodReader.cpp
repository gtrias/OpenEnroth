#include "LodReader.h"

#include <cassert>
#include <utility>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <vector>

#include "Library/Compression/Compression.h"
#include "Library/FileSystem/Interface/FileSystem.h"
#include "Library/Snapshots/SnapshotSerialization.h"

#include "Utility/Streams/BlobInputStream.h"
#include "Utility/Exception.h"
#include "Utility/String/Ascii.h"

#include "LodSnapshots.h"
#include "LodEnums.h"

static LodHeader parseHeader(InputStream &stream, LodVersion *version) {
    LodHeader header;
    deserialize(stream, &header, tags::via<LodHeader_MM6>);

    if (header.signature != "LOD")
        throw Exception("File '{}' is not a valid LOD: expected signature '{}', got '{}'", stream.displayPath(), "LOD", ascii::toPrintable(header.signature));

    if (!tryDeserialize(header.version, version))
        throw Exception("File '{}' is not a valid LOD: version '{}' is not recognized", stream.displayPath(), ascii::toPrintable(header.version));

    // While LOD structure itself support multiple directories, all LOD files associated with
    // vanilla MM6/7/8 games use a single directory.
    if (header.numDirectories != 1)
        throw Exception("File '{}' is not a valid LOD: expected a single directory, got '{}' directories", stream.displayPath(), header.numDirectories);

    return header;
}

static LodEntry parseDirectoryEntry(InputStream &stream, LodVersion version, size_t lodSize) {
    LodEntry result;
    deserialize(stream, &result, tags::via<LodEntry_MM6>);

    size_t expectedDataSize = result.numItems * fileEntrySize(version);
    if (result.dataSize < expectedDataSize)
        throw Exception("File '{}' is not a valid LOD: invalid root directory index size, expected at least {} bytes, got {} bytes", stream.displayPath(), expectedDataSize, result.dataSize);

    if (result.dataOffset + result.dataSize > lodSize)
        throw Exception("File '{}' is not a valid LOD: root directory index points outside the LOD file", stream.displayPath());

    return result;
}

static std::vector<LodEntry> parseFileEntries(InputStream &stream, const LodEntry &directoryEntry, LodVersion version) {
    std::vector<LodEntry> result;
    if (version == LOD_VERSION_MM8) {
        deserialize(stream, &result, tags::presized(directoryEntry.numItems), tags::each, tags::via<LodFileEntry_MM8>);
    } else {
        deserialize(stream, &result, tags::presized(directoryEntry.numItems), tags::each, tags::via<LodEntry_MM6>);
    }

    for (const LodEntry &entry : result) {
        if (entry.numItems != 0)
            throw Exception("File '{}' is not a valid LOD: subdirectories are not supported, but '{}' is a subdirectory", stream.displayPath(), entry.name);
        if (entry.dataOffset + entry.dataSize > directoryEntry.dataSize)
            throw Exception("File '{}' is not a valid LOD: entry '{}' points outside the LOD file", stream.displayPath(), entry.name);
    }

    return result;
}


LodReader::LodReader() = default;

LodReader::LodReader(const NativePath &path, LodOpenFlags openFlags) {
    open(path, openFlags);
}

LodReader::LodReader(Blob blob, LodOpenFlags openFlags) {
    open(std::move(blob), openFlags);
}

LodReader::~LodReader() {
    close();
}

void LodReader::open(const NativePath &path, LodOpenFlags openFlags) {
    open(Blob::fromFile(path), openFlags); // Blob::fromFile throws if the file doesn't exist.
}

void LodReader::open(Blob blob, LodOpenFlags openFlags) {
    close();

    size_t expectedSize = sizeof(LodHeader_MM6) + sizeof(LodEntry_MM6); // Header + directory entry.
    if (blob.size() < expectedSize)
        throw Exception("File '{}' is not a valid LOD: expected file size at least {} bytes, got {} bytes", blob.displayPath(), expectedSize, blob.size());

    BlobInputStream lodStream(blob);
    LodVersion version = LOD_VERSION_MM6;
    LodHeader header = parseHeader(lodStream, &version);
    LodEntry rootEntry = parseDirectoryEntry(lodStream, version, blob.size());

    // LODs that come with the Russian version of MM7 are broken.
    rootEntry.dataSize = blob.size() - rootEntry.dataOffset;

    BlobInputStream dirStream(blob.subBlob(rootEntry.dataOffset, rootEntry.dataSize).withDisplayPath(blob.displayPath()));
    indexFiles(parseFileEntries(dirStream, rootEntry, version), rootEntry, openFlags, blob.displayPath());

    // All good, this is a valid LOD, can update `this`.
    _lod = std::move(blob);
    _info.version = version;
    _info.description = std::move(header.description);
    _info.rootName = std::move(rootEntry.name);
}

void LodReader::open(FileSystem *fs, std::string_view path, LodOpenFlags openFlags) {
    assert(fs);

    close();

    std::unique_ptr<InputStream> stream = fs->openForReading(path);
    std::string displayPath = stream->displayPath();

    std::int64_t fileSize = stream->size();
    if (fileSize < 0)
        throw Exception("File '{}' is not a valid LOD: its size is unknown", displayPath);

    size_t expectedSize = sizeof(LodHeader_MM6) + sizeof(LodEntry_MM6); // Header + directory entry.
    if (fileSize < static_cast<std::int64_t>(expectedSize))
        throw Exception("File '{}' is not a valid LOD: expected file size at least {} bytes, got {} bytes", displayPath, expectedSize, fileSize);

    LodVersion version = LOD_VERSION_MM6;
    LodHeader header = parseHeader(*stream, &version);
    LodEntry rootEntry = parseDirectoryEntry(*stream, version, fileSize);

    // LODs that come with the Russian version of MM7 are broken.
    rootEntry.dataSize = fileSize - rootEntry.dataOffset;

    // Only the index is read into memory - note that `rootEntry.dataSize` spans to the end of the file after the fixup
    // above, so the file entries are read on demand in read() instead.
    size_t indexSize = rootEntry.numItems * fileEntrySize(version);
    stream->skipOrFail(rootEntry.dataOffset - stream->position());
    Blob directory = Blob::read(stream.get(), indexSize).withDisplayPath(displayPath);

    std::vector<LodEntry> entries = [&directory, &rootEntry, version] {
        BlobInputStream dirStream(directory);
        return parseFileEntries(dirStream, rootEntry, version);
    }();
    indexFiles(std::move(entries), rootEntry, openFlags, displayPath);

    // All good, this is a valid LOD, can update `this`.
    _fs = fs;
    _path = std::string(path);
    _displayPath = std::move(displayPath);
    _info.version = version;
    _info.description = std::move(header.description);
    _info.rootName = std::move(rootEntry.name);
}

void LodReader::indexFiles(std::vector<LodEntry> entries, const LodEntry &rootEntry, LodOpenFlags openFlags, std::string_view displayPath) {
    for (const LodEntry &entry : entries) {
        std::string name = ascii::toLower(entry.name);
        if (_files.contains(name)) {
            if (openFlags & LOD_ALLOW_DUPLICATES) {
                continue; // Only the first entry is kept in this case.
            } else {
                throw Exception("File '{}' is not a valid LOD: contains duplicate entries for '{}'", displayPath, name);
            }
        }

        LodRegion region;
        region.offset = rootEntry.dataOffset + entry.dataOffset;
        region.size = entry.dataSize;
        _files.emplace(std::move(name), region);
    }
}

void LodReader::close() {
    // Double-closing is OK.
    _lod = Blob();
    _fs = nullptr;
    _path = {};
    _displayPath = {};
    _info = {};
    _files = {};
}

bool LodReader::exists(std::string_view filename) const {
    assert(isOpen());

    return _files.contains(ascii::toLower(filename));
}

Blob LodReader::read(std::string_view filename) const {
    assert(isOpen());

    const auto pos = _files.find(ascii::toLower(filename));
    if (pos == _files.cend())
        throw Exception("Entry '{}' doesn't exist in LOD file '{}'", filename, lodDisplayPath());

    if (_lod)
        return _lod.subBlob(pos->second.offset, pos->second.size).withDisplayPath(displayPath(filename));

    std::unique_ptr<InputStream> stream = _fs->openForReading(_path);
    stream->skipOrFail(pos->second.offset);
    return Blob::read(stream.get(), pos->second.size).withDisplayPath(displayPath(filename));
}

std::string LodReader::lodDisplayPath() const {
    return _lod ? _lod.displayPath() : _displayPath;
}

std::string LodReader::displayPath(std::string_view filename) const {
    return fmt::format("{}/{}", lodDisplayPath(), filename);
}

std::vector<std::string> LodReader::ls() const {
    assert(isOpen());

    std::vector<std::string> result;
    for (const auto &[name, _] : _files)
        result.push_back(name);
    std::sort(result.begin(), result.end());
    return result;
}

[[nodiscard]] const LodInfo &LodReader::info() const {
    assert(isOpen());

    return _info;
}

bool lod::detect(const Blob &data) {
    if (data.size() < sizeof(LodHeader_MM6) + sizeof(LodEntry_MM6)) // Header + directory entry.
        return false;

    BlobInputStream stream(data);
    LodHeader header;
    deserialize(stream, &header, tags::via<LodHeader_MM6>);

    if (header.signature != "LOD")
        return false;

    LodVersion version;
    if (!tryDeserialize(header.version, &version))
        return false;

    // While LOD structure itself support multiple directories, all LOD files associated with
    // vanilla MM6/7/8 games use a single directory.
    if (header.numDirectories != 1)
        return false;

    return true;
}
