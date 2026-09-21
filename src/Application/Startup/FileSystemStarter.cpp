#include "FileSystemStarter.h"

#include <memory>
#include <vector>
#include <utility>

#include "Library/FileSystem/Directory/DirectoryFileSystem.h"
#include "Library/FileSystem/Embedded/EmbeddedFileSystem.h"
#include "Library/FileSystem/Lowercase/LowercaseFileSystem.h"
#ifdef __vita__
#include "Library/FileSystem/Vita/VitaFileSystem.h"
#endif
#include "Library/FileSystem/Merging/MergingFileSystem.h"
#include "Library/FileSystem/Memory/MemoryFileSystem.h"

#include "Engine/Resources/EngineFileSystem.h"

CMRC_DECLARE(openenroth);

FileSystemStarter::FileSystemStarter() = default;

FileSystemStarter::~FileSystemStarter() {
    ufs = nullptr;
    dfs = nullptr;
}

void FileSystemStarter::initUserFs(bool ramFs, const NativePath &path) {
    assert(ufs == nullptr);

    if (ramFs) {
        _userFs = std::make_unique<MemoryFileSystem>("ramfs");
    } else {
#ifdef __vita__
        _userFs = std::make_unique<VitaFileSystem>(path.toWtf8());
#else
        _userFs = std::make_unique<DirectoryFileSystem>(path);
#endif
    }

    ufs = _userFs.get();
}

void FileSystemStarter::initDataFs(const NativePath &path, bool pathOverridesBuiltIn) {
    assert(dfs == nullptr);

    _dataEmbeddedFs = std::make_unique<EmbeddedFileSystem>(cmrc::openenroth::get_filesystem(), "embedded");
#ifdef __vita__
    _dataDirFs = std::make_unique<VitaFileSystem>(path.toWtf8());
#else
    _dataDirFs = std::make_unique<DirectoryFileSystem>(path);
#endif
    _dataDirLowercaseFs = std::make_unique<LowercaseFileSystem>(_dataDirFs.get());

    std::vector<const FileSystem *> baseFileSystems = {_dataDirLowercaseFs.get(), _dataEmbeddedFs.get()};
    if (!pathOverridesBuiltIn)
        std::swap(baseFileSystems[0], baseFileSystems[1]);
    _dataFs = std::make_unique<MergingFileSystem>(std::move(baseFileSystems));

    dfs = _dataFs.get();
}
