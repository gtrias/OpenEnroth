#include "VitaFileSystem.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include "Library/FileSystem/Interface/FileSystemException.h"

#include "Library/Logger/Logger.h"

#include "Utility/Exception.h"
#include "Utility/Streams/FileInputStream.h"
#include "Utility/Streams/FileOutputStream.h"

VitaFileSystem::VitaFileSystem(std::string root) : _root(std::move(root)) {
    while (_root.size() > 1 && _root.back() == '/')
        _root.pop_back();
}

VitaFileSystem::~VitaFileSystem() = default;

bool VitaFileSystem::_exists(FileSystemPathView path) const {
    assert(!path.isEmpty());
    return !!_stat(path);
}

FileStat VitaFileSystem::_stat(FileSystemPathView path) const {
    assert(!path.isEmpty());

    SceIoStat stat = {};
    if (sceIoGetstat(makePath(path).c_str(), &stat) < 0)
        return {};
    if (SCE_S_ISREG(stat.st_mode))
        return {FILE_REGULAR, stat.st_size};
    if (SCE_S_ISDIR(stat.st_mode))
        return {FILE_DIRECTORY, 0};
    return {};
}

void VitaFileSystem::_ls(FileSystemPathView path, std::vector<DirectoryEntry> *entries) const {
    FileStat parent = path.isEmpty() ? FileStat(FILE_DIRECTORY, 0) : _stat(path);
    if (!parent) {
        if (path.isEmpty())
            return; // ls("") always succeeds, even when the underlying root does not exist.
        FileSystemException::raise(this, FS_LS_FAILED_PATH_DOESNT_EXIST, path);
    }
    if (parent.type != FILE_DIRECTORY)
        FileSystemException::raise(this, FS_LS_FAILED_PATH_IS_FILE, path);

    SceUID directory = sceIoDopen(makePath(path).c_str());
    if (directory < 0) {
        if (path.isEmpty())
            return;
        FileSystemException::raise(this, FS_LS_FAILED_PATH_DOESNT_EXIST, path);
    }

    SceIoDirent entry = {};
    while (sceIoDread(directory, &entry) > 0) {
        std::string name = entry.d_name;
        if (name != "." && name != ".." && name.find('\\') == std::string::npos) {
            FileType type = FILE_INVALID;
            if (SCE_S_ISREG(entry.d_stat.st_mode))
                type = FILE_REGULAR;
            else if (SCE_S_ISDIR(entry.d_stat.st_mode))
                type = FILE_DIRECTORY;
            if (type != FILE_INVALID)
                entries->emplace_back(std::move(name), type);
        }
        entry = {};
    }
    (void) sceIoDclose(directory);
}

Blob VitaFileSystem::_read(FileSystemPathView path) const {
    assert(!path.isEmpty());
    FileStat stat = _stat(path);
    if (stat.type != FILE_REGULAR)
        FileSystemException::raise(this, FS_READ_FAILED_PATH_DOESNT_EXIST, path);

    // Reading goes through a single malloc of the whole file (Blob::read), and this port's memory budget is tight -
    // so log the large reads, and reject sizes that can only come from a broken stat.
    constexpr std::int64_t LARGE_READ = 4 * 1024 * 1024;
    constexpr std::int64_t MAX_PLAUSIBLE_SIZE = 512ll * 1024 * 1024;
    if (stat.size > MAX_PLAUSIBLE_SIZE)
        FileSystemException::raise(this, FS_READ_FAILED_PATH_DOESNT_EXIST, path);
    if (stat.size > LARGE_READ)
        MM_INFO("Reading '{}' ({} KiB).", path.string(), stat.size / 1024);

    std::unique_ptr<InputStream> stream = _openForReading(path);
    return Blob::read(stream.get(), stat.size);
}

void VitaFileSystem::_write(FileSystemPathView path, const Blob &data) {
    assert(!path.isEmpty());
    std::unique_ptr<OutputStream> stream = _openForWriting(path);
    stream->write(data);
    stream->close();
}

std::unique_ptr<InputStream> VitaFileSystem::_openForReading(FileSystemPathView path) const {
    assert(!path.isEmpty());
    return std::make_unique<FileInputStream>(NativePath::fromWtf8(makePath(path)));
}

std::unique_ptr<OutputStream> VitaFileSystem::_openForWriting(FileSystemPathView path) {
    assert(!path.isEmpty());
    makeParentDirectories(path);
    return std::make_unique<FileOutputStream>(NativePath::fromWtf8(makePath(path)));
}

bool VitaFileSystem::_remove(FileSystemPathView path) {
    assert(!path.isEmpty());
    return removeTree(path);
}

std::string VitaFileSystem::_displayPath(FileSystemPathView path) const {
    return makePath(path);
}

std::string VitaFileSystem::makePath(FileSystemPathView path) const {
    if (path.isEmpty())
        return _root;
    std::string result = _root;
    result += '/';
    result += path.string();
    return result;
}

void VitaFileSystem::makeParentDirectories(FileSystemPathView path) const {
    std::string fullPath = makePath(path);
    size_t pos = _root.size();
    while ((pos = fullPath.find('/', pos + 1)) != std::string::npos) {
        std::string directory = fullPath.substr(0, pos);
        (void) sceIoMkdir(directory.c_str(), 0777); // Existing directories return an error, which is benign.
    }
}

bool VitaFileSystem::removeTree(FileSystemPathView path) {
    FileStat stat = _stat(path);
    if (!stat)
        return false;

    std::string fullPath = makePath(path);
    if (stat.type == FILE_REGULAR)
        return sceIoRemove(fullPath.c_str()) >= 0;

    SceUID directory = sceIoDopen(fullPath.c_str());
    if (directory < 0)
        return false;

    SceIoDirent entry = {};
    while (sceIoDread(directory, &entry) > 0) {
        std::string name = entry.d_name;
        if (name != "." && name != "..") {
            FileSystemPath child(path);
            child /= name;
            (void) removeTree(child);
        }
        entry = {};
    }
    (void) sceIoDclose(directory);
    return sceIoRmdir(fullPath.c_str()) >= 0;
}
