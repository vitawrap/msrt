#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace ms {
namespace io {

    typedef void* TFileHandle;

    typedef std::unique_ptr<class File> TFile;

    struct FileSystem {
        static int s_status;

        /**
         * Test natively if a file exists
         */
        static bool exists(char const* filename);

        /**
         * Test natively if a directory exists
         */
        static bool existsDir(char const* pathname);

        /**
         * Open file wrapped in auto-closing pointer.
         * Useful to avoid descriptor leaks.
         * @param filename Relative FS filename
         */
        static TFile openForRead(char const* filename);

        /**
         * Read entire file as (ideally printable) text
         * @param filename Relative FS filename
         */
        static std::string readString(char const* filename);
    };

    /**
     * Simple extendable wrapper over POSIX file API
     */
    class File {
    protected:
        std::filesystem::path m_path;
        // TFileHandle m_handle;
        // static int s_status;

        // File(TFileHandle handle, char const* path)
        //     : m_handle(handle)
        //     , m_path(path)
        // {}

        File(char const* path)
            : m_path(path)
        {}

    public:
        enum Constants {
            OK
        };

        virtual ~File() {}

        /**
         * Get byte position of file cursor
         */
        virtual size_t at() const = 0;

        /**
         * Get open file stream size
         */
        virtual size_t size() const = 0;

        /**
         * Simple EOF check
         */
        bool eof() const { return at() == size(); }

        /**
         * Get status of last opened file
         */
        virtual int status() const { return OK; };

        /**
         * fread wrapper, read raw binary chunk
         * @param ptr Raw output data buffer
         * @param sz Size of data chunk to read
         * @param count How many times to repeat read
         * @returns Number of bytes that could be read
         */
        virtual size_t read(void* ptr, size_t sz, size_t count = 1) = 0;

        /**
         * fgets wrapper, buffer contents until newline (included)
         * @param ptr Output text buffer
         * @param sz Size of text buffer (reads at most sz - 1)
         * @returns Number of characters read including newline
         */
        virtual size_t readLine(char* ptr, size_t sz) = 0;

        /**
         * Read entire file as (ideally printable) text, on instance.
         */
        virtual std::string readString() = 0;

        /**
         * Get the path for this file.
         */
        std::filesystem::path const& path() const { return m_path; }
    };

    /**
     * @brief Filesystem file handle
     * Simple extendable wrapper over POSIX file API
     */
    class FSFile : public File {
        friend class FileSystem;
        TFileHandle m_handle;

        FSFile(TFileHandle handle, char const* path)
            : m_handle(handle)
            , File(path)
        {}

    public:
        ~FSFile();

        size_t read(void *ptr, size_t sz, size_t count = 1) override;
        size_t readLine(char *ptr, size_t sz) override;
        std::string readString() override;
        size_t at() const override;
        size_t size() const override;
        int status() const override;
    };

    /**
     * @brief File stream stored in memory, good for zip files and such.
     * This class does not free up the buffer it is given!
     */
    class MemFile : public File {
        char* m_buffer;
        size_t m_size;
        size_t m_pos;

    public:
        ~MemFile() {}

        MemFile(char const* path, char* buffer = nullptr, size_t size = 0ull) :
            m_buffer(buffer),
            m_size(size),
            m_pos(0ull),
            File(path)
        {}

        size_t read(void *ptr, size_t sz, size_t count = 1) override;
        size_t readLine(char *ptr, size_t sz) override;
        std::string readString() override;
        size_t at() const override { return m_pos; }
        size_t size() const override { return m_size; }
        //int status() const override;
    };

}
}
