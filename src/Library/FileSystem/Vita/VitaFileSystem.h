#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Library/FileSystem/Interface/FileSystem.h"

/**
 * Rooted filesystem for Vita device paths. Unlike DirectoryFileSystem, this
 * never uses std::filesystem: VitaSDK's implementation cannot reliably walk
 * device paths such as ux0:/data/OpenEnroth.
 */
class VitaFileSystem final : public FileSystem {
 public:
    explicit VitaFileSystem(std::string root);
    ~VitaFileSystem() override;

 private:
    bool _exists(FileSystemPathView path) const override;
    FileStat _stat(FileSystemPathView path) const override;
    void _ls(FileSystemPathView path, std::vector<DirectoryEntry> *entries) const override;
    Blob _read(FileSystemPathView path) const override;
    void _write(FileSystemPathView path, const Blob &data) override;
    std::unique_ptr<InputStream> _openForReading(FileSystemPathView path) const override;
    std::unique_ptr<OutputStream> _openForWriting(FileSystemPathView path) override;
    bool _remove(FileSystemPathView path) override;
    std::string _displayPath(FileSystemPathView path) const override;

    std::string makePath(FileSystemPathView path) const;
    void makeParentDirectories(FileSystemPathView path) const;
    bool removeTree(FileSystemPathView path);

 private:
    std::string _root;
};
