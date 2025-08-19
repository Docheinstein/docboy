#ifndef UTILSTRAVERSAL_H
#define UTILSTRAVERSAL_H

#include <cstdint>
#include <cstring>
#include <string>

// TODO: Support for Windows

#ifndef WINDOWS

#include "dirent.h"

struct DirectoryIteratorEntry {
    enum FileType : uint8_t {
        File = (1 << 0),
        Directory = (1 << 1),
        Other = (1 << 2),
    };

    const char* filename {};
    FileType type {};
};

struct DirectorySentinel {};

struct DirectoryIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = DirectoryIteratorEntry;
    using difference_type = std::ptrdiff_t;
    using pointer = const DirectoryIteratorEntry*;
    using reference = const DirectoryIteratorEntry&;

    explicit DirectoryIterator(const char* path) {
        dir = opendir(path);
        advance();
    }

    ~DirectoryIterator() noexcept {
        if (dir) {
            closedir(dir);
        }
    }

    DirectoryIterator(const DirectoryIterator&) = delete;
    DirectoryIterator& operator=(const DirectoryIterator&) = delete;

    DirectoryIterator(DirectoryIterator&& other) noexcept :
        dir {other.dir},
        entry {other.entry},
        current {other.current} {
        other.dir = nullptr;
        other.entry = nullptr;
    }

    DirectoryIterator& operator=(DirectoryIterator&& other) noexcept {
        if (this != &other) {
            if (dir) {
                closedir(dir);
            }

            dir = other.dir;
            entry = other.entry;
            current = other.current;

            other.dir = nullptr;
            other.entry = nullptr;
        }

        return *this;
    }

    reference operator*() const {
        return current;
    };

    pointer operator->() const {
        return &current;
    };

    DirectoryIterator& operator++() {
        advance();
        return *this;
    }

    bool operator!=(DirectorySentinel) const {
        return entry != nullptr;
    }

private:
    void advance() {
        if (!dir) {
            return;
        }

        do {
            entry = readdir(dir);
        } while (entry && (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0));

        if (entry) {
            current.filename = entry->d_name;
            switch (entry->d_type) {
            case DT_REG:
                current.type = DirectoryIteratorEntry::FileType::File;
                break;
            case DT_DIR:
                current.type = DirectoryIteratorEntry::FileType::Directory;
                break;
            default:
                current.type = DirectoryIteratorEntry::FileType::Other;
            }
        } else {
            closedir(dir);
            dir = nullptr;
        }
    }

    DIR* dir {};
    dirent* entry {};

    value_type current {};
};

struct DirectoryTraverser {
    explicit DirectoryTraverser(std::string path) :
        path {std::move(path)} {
    }

    DirectoryIterator begin() {
        return DirectoryIterator {path.c_str()};
    }

    DirectorySentinel end() {
        return DirectorySentinel {};
    }

private:
    std::string path {};
};
#endif

#endif // UTILSTRAVERSAL_H
