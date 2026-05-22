#pragma once

#include "io/zip_archive.hpp"
#include "resources/ms_script.hpp"
#include "resources/ms_image.hpp"
#include "resources/resource_manager.hpp"

namespace ms {
namespace res {

    class Project {
    public:
        struct Settings {
            std::string title;
            std::string orientation;
            std::string aspect;
        };

        robin_hood::unordered_map<
            std::string,
            res::ResourceHandle<res::Script>
        > m_scripts;

        robin_hood::unordered_map<
            std::string,
            res::ResourceHandle<res::Image>
        > m_sprites;

        ResourceHandle<Image> m_atlas;
        ResourceHandle<Image> m_icon;

    private:
        Settings m_settings;

        /**
         * Archive must always be available since microscript can
         * query the project for any asset at runtime.
         */
        io::ZipArchive m_files;

    public:
        Project():
            m_atlas{nullptr}
        {}

        bool open(char const* filename);

        void close();

        bool isOpen() const { return m_files.isOpen(); }
        
        /** Get settings associated with this project */
        Settings const& getSettings() const { return m_settings; }

        /** Get map of loaded scripts */
        decltype(m_scripts) const& getScriptMap() const { return m_scripts; }

        /** Get map of loaded sprites */
        decltype(m_sprites) const& getSpriteMap() const { return m_sprites; }

        /** Get atlas constructed with the sprites from this project */
        ResourceHandle<Image> getSpriteAtlas() const { return m_atlas; }

        /** Get sprite defined as the project icon */
        ResourceHandle<Image> getIcon() const { return m_icon; }

        /** Build atlas out of project sprites */
        void buildAtlas();
    };

}
}
