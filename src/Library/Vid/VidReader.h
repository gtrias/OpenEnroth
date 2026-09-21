#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Utility/Memory/Blob.h"
#include "Utility/System/NativePath.h"

class FileSystem;
struct VidEntry;

/**
 * Reader for Might&Magic VID files.
 */
class VidReader {
 public:
    VidReader();
    explicit VidReader(const NativePath &path);
    explicit VidReader(Blob blob);
    ~VidReader();

    /**
     * @param path                      Path to the VID file to open for reading.
     * @throw Exception                 If the VID couldn't be opened - e.g., if the file doesn't exist,
     *                                  or if it's not in VID format.
     */
    void open(const NativePath &path);

    /**
     * @param blob                      VID data.
     * @throw Exception                 If there are errors in the provided VID file.
     */
    void open(Blob blob);

    /**
     * Opens a VID without keeping it in memory: only the index is read up front, and entries are read from `fs` on
     * demand. This is what platforms with a tight memory budget use - `might7.vid` alone is 110 MiB, more than the
     * Vita has free for the engine.
     *
     * @param fs                        Filesystem to read the VID from.
     * @param path                      Path to the VID file, relative to `fs`.
     * @throw Exception                 If the VID couldn't be opened, or if it's not a valid VID.
     */
    void open(FileSystem *fs, std::string_view path);

    /**
     * Closes this VID reader & frees all associated resources.
     */
    void close();

    [[nodiscard]] bool isOpen() const {
        return !!_vid || _fs;
    }

    /**
     * @param filename                  Name of the VID file entry.
     * @return                          Whether the file exists inside the VID. The check is case-insensitive.
     */
    [[nodiscard]] bool exists(std::string_view filename) const;

    /**
     * @param filename                  Name of the VID file entry.
     * @return                          Contents of the file inside the VID as a `Blob`.
     * @throws Exception                If file doesn't exist inside the VID.
     */
    [[nodiscard]] Blob read(std::string_view filename) const;

    /**
     * @return                          List of all files in the VID.
     */
    [[nodiscard]] std::vector<std::string> ls() const;

 private:
    struct VidRegion {
        size_t offset = 0;
        size_t size = 0;
    };

 private:
    // Fills `_files` from a parsed index, throwing if the VID is malformed.
    void indexFiles(std::vector<VidEntry> entries, std::int64_t fileSize, std::string_view displayPath);

    // Path of the VID itself, for messages - uses the blob's path when loaded into memory, and the stored path when
    // streaming.
    [[nodiscard]] std::string vidDisplayPath() const;

 private:
    Blob _vid;
    FileSystem *_fs = nullptr; // Set when opened for streaming, in which case `_vid` is empty.
    std::string _path;         // VID path inside `_fs`, streaming mode only.
    std::string _displayPath;  // VID display path, streaming mode only.
    std::unordered_map<std::string, VidRegion> _files;
};

namespace vid {
bool detect(const Blob &data);
} // namespace vid
