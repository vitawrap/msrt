#include "file.hpp"
#include <algorithm>
#include <cstddef>
#include <errno.h>
#include <memory>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/stat.h>

// Platform-specific define
#include "platform/platform.hpp"

namespace ms {
namespace io {

    size_t FSFile::read(void *ptr, size_t sz, size_t count) {
        return fread(ptr, sz, count, (FILE *)m_handle);
    }

    size_t FSFile::readLine(char *ptr, size_t sz) {
        char* result = fgets(ptr, sz, (FILE*)m_handle);
        if (!result)
        {
            ptr[0] = 0; // make sure it still passes as a string, assume buffer size >= 1
            return 0;
        }
        return strlen(result);
    }

    size_t FSFile::size() const {
        size_t at = ftell((FILE* )m_handle);
        FileSystem::s_status = fseek((FILE*)m_handle, 0, SEEK_END);
        if (FileSystem::s_status != File::OK)
            return -1;
        size_t size = ftell((FILE*)m_handle);
        FileSystem::s_status = fseek((FILE*)m_handle, at, SEEK_SET);
        return size;
    }

    size_t FSFile::at() const {
        return ftell((FILE*)m_handle);
    }

    int FSFile::status() const {
        int err = FileSystem::s_status;
        FileSystem::s_status = File::OK;
        return err;
    }

    FSFile::~FSFile() {
        fclose((FILE*) m_handle);
        m_handle = 0;
    }

    // TODO: This does not account for at() offset!
    std::string FSFile::readString() {
        if (FileSystem::s_status != File::OK)
            return std::string{};

        size_t strSize = size();
        char* strBuf = new char[strSize];
        read(strBuf, strSize);

        std::string str(strBuf, strSize);
        delete [] strBuf;
        return str;
    }

    size_t MemFile::read(void *ptr, size_t sz, size_t count) {
        size_t actual = std::min(size() - at(), sz * count);
        memcpy(ptr, m_buffer + at(), actual);
        return actual;
    }

    size_t MemFile::readLine(char *ptr, size_t sz) {
        size_t actual = std::min(size() - at(), sz - 1);
        char* c = (char*) memccpy(ptr, m_buffer + at(), '\n', actual);
        if (c) actual = size_t(c - ptr);
        ptr[actual] = 0;
        return actual;
    }

    std::string MemFile::readString() {
        if (FileSystem::s_status != File::OK)
            return std::string{};
        
        // we can simply use the in-memory buffer here
        return std::string(m_buffer + at(), size() - at());
    }

    int FileSystem::s_status = 0;

    bool FileSystem::exists(char const* filename) {
        struct stat buffer;
        return stat(filename, &buffer) == 0;
    }

    bool FileSystem::existsDir(char const* pathname) {
        struct stat buffer;
        if (stat(pathname, &buffer) == 0)
            return (buffer.st_mode & S_IFMT) == S_IFDIR;
        return false;
    }

    TFile FileSystem::openForRead(const char *filename) {
    #ifdef PLATFORM_NT
        FILE* ptr = nullptr;
        s_status = (int) fopen_s(&ptr, filename, "rb");
        if (s_status != CFile::OK)
            return TFile(nullptr);
        return TFile(new CFile((TFileHandle) ptr, filename));
    #else
        FILE* ptr = fopen(filename, "rb");
        if (!ptr) {
            s_status = errno;
            return TFile(nullptr);
        }
        return TFile(new FSFile((TFileHandle) ptr, filename));
    #endif
    }

    std::string FileSystem::readString(const char *filename) {
        auto fp = openForRead(filename);
        return fp->readString();
    }

}
}

