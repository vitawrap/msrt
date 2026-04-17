#pragma once

#include "io/zip_archive.hpp"

namespace ms {
namespace res {

    class Project {
    public:
        struct Settings {
            std::string title;
        };

    private:
        Settings m_settings;

        /**
         * Archive must always be available since microscript can
         * query the project for any asset at runtime.
         */
        io::ZipArchive m_files;

    public:

        bool open(char const* filename);

        void close();

        bool isOpen() const { return m_files.isOpen(); }
        
        /** Get settings associated with this project */
        Settings const& getSettings() const { return m_settings; }
    };

}
}
