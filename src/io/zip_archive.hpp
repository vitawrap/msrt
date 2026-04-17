#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ms {
namespace io {

    /**
     * @brief RAII interface for minizip (unzip)
     * The path format for methods accepting a path is:
     * aaa/bbb/ccc
     * along with an additional trailing slash if the path is a dir.
     */
    class ZipArchive {
        void* m_pArchive;
        mutable int m_lastError;

    public:
        ZipArchive() :
            m_pArchive(nullptr),
            m_lastError(0)
        {}

        ~ZipArchive();

        /**
         * @brief Open a new archive, automatically closing the previous one.
         * @param filename Filename on disk of the archive to open
         * @return true if no error occured, false otherwise
         */
        bool open(char const* filename);

        /**
         * @brief Close the currently open archive if any.
         */
        void close();

        /**
         * @brief Get the complete list of files in the archive.
         * @param list contains the paths of all files within the archive
         * @return true if no error occured and the list is filled, false otherwise
         */
        bool getFileList(std::vector<std::string>& list) const;

        /**
         * @brief Check for file in ZIP.
         * @param filepath path of file in archive
         * @param caseSens controls case sensitivity of search path
         * @return whether or not the file exists in the archive
         */
        bool fileExists(char const* filepath, bool caseSens = true) const;

        /**
         * @brief Read file from ZIP.
         * @param filepath path of file in archive
         * @param caseSens controls case sensitivity of search path
         * @return unique pointer to data, automatically deallocated when out of scope
         */
        std::unique_ptr<char[]> readFile(char const* filepath, size_t& sz, bool caseSens = true) const;

        /**
         * @brief Is there a currently open archive?
         */
        bool isOpen() const { return !!m_pArchive; }

        /**
         * @brief Is the archive reader in error state?
         */
        bool isOK() const { return m_lastError == 0; }

        /**
         * @brief Query the internal UNZ_* error code.
         */
        int getLastError() const { return m_lastError; }
    };

}
}