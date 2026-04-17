#include "zip_archive.hpp"
#include <minizip/unzip.h>

namespace ms {
namespace io {

    ZipArchive::~ZipArchive() {
        close();
    }

    bool ZipArchive::open(char const* filename) {
        close();

        m_pArchive = unzOpen(filename);
        return !!m_pArchive;
    }

    void ZipArchive::close() {
        if (!m_pArchive) return;

        m_lastError = unzClose(m_pArchive);
        m_pArchive = nullptr;
    }

    bool ZipArchive::getFileList(std::vector<std::string>& fileList) const {
        if (!m_pArchive) return false; // TODO: Exceptions?

        unz_global_info info;
        m_lastError = unzGetGlobalInfo(m_pArchive, &info);
        if (m_lastError != UNZ_OK)
            return false;
        
        m_lastError = unzGoToFirstFile(m_pArchive);
        if (m_lastError != UNZ_OK)
            return false;
        
        do {
            char pathNameBuffer[2048];
            unz_file_info64 file;
            m_lastError = unzGetCurrentFileInfo64(m_pArchive, &file, pathNameBuffer, 2048, nullptr, 0, nullptr, 0);
            if (m_lastError != UNZ_OK)
                return false;
            
            fileList.push_back(pathNameBuffer);
        } while (unzGoToNextFile(m_pArchive) == UNZ_OK);

        return true;
    }

    bool ZipArchive::fileExists(char const* filepath, bool caseSens) const {
        if (!m_pArchive) return false;

        if ((m_lastError = unzLocateFile(m_pArchive, filepath, caseSens ? 1 : 2)) != UNZ_OK)
            return false;
        if ((m_lastError = unzOpenCurrentFile(m_pArchive)) != UNZ_OK)
            return false;
        
        unzCloseCurrentFile(m_pArchive);
        return true;
    }

    std::unique_ptr<char[]> ZipArchive::readFile(char const* filepath, size_t& sz, bool caseSens) const {
        if (!m_pArchive) return nullptr;

        if ((m_lastError = unzLocateFile(m_pArchive, filepath, caseSens ? 1 : 2)) != UNZ_OK)
            return nullptr;
        if ((m_lastError = unzOpenCurrentFile(m_pArchive)) != UNZ_OK)
            return nullptr;
        
        unz_file_info info;
        if ((m_lastError = unzGetCurrentFileInfo(m_pArchive, &info, nullptr, 0, nullptr, 0, nullptr, 0)) != UNZ_OK)
            return nullptr;
        if (info.uncompressed_size > INT_MAX)
            return nullptr;

        size_t bytesLeft = sz = info.uncompressed_size;
        auto ptr = std::make_unique<char[]>(bytesLeft);
        char* buffer = ptr.get();
        while (bytesLeft > 0) {
            int bytesRead = unzReadCurrentFile(m_pArchive, buffer, bytesLeft);
            if (bytesRead < 0) return nullptr;
            if (bytesRead == UNZ_EOF && bytesLeft != 0) return nullptr;
            buffer += bytesRead;
            bytesLeft -= bytesRead;
        }
        m_lastError = unzCloseCurrentFile(m_pArchive);

        return std::move(ptr);
    }

}
}
