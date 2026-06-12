#pragma once

#include "io/zip_archive.hpp"
#include "resources/ms_script.hpp"
#include "resources/ms_image.hpp"
#include "resources/ms_tilemap.hpp"
#include "resources/resource_manager.hpp"

namespace ms {
namespace res {

    class Project {
    public:
        struct SpriteSheet {
            std::string filename;
            int nframes;
            int nfps;
        };
    
        struct Settings {
            std::string title;
            std::string orientation;
            std::string aspect;
            std::string language;
            std::vector<SpriteSheet> spritesheets;
        };

        enum Language {
            L_Unknown,
            L_MicroscriptV2,
            L_JavaScript,
        };

        robin_hood::unordered_map<
            std::string,
            res::ResourceHandle<res::Script>
        > m_scripts;

        robin_hood::unordered_map<
            std::string,
            res::ResourceHandle<res::Image>
        > m_sprites;

        robin_hood::unordered_map<
            std::string,
            res::ResourceHandle<res::TileMap>
        > m_tilemaps;

        ResourceHandle<Image> m_atlas;
        ResourceHandle<Image> m_icon;

    private:
        Settings m_settings;

        /**
         * Archive must always be available since microscript can
         * query the project for any asset at runtime.
         */
        io::ZipArchive m_files;

        SpriteSheet const* findSheetInfo(std::string_view path) const;

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

        /** Get map of loaded tilemaps */
        decltype(m_tilemaps) const& getTileMapMap() const { return m_tilemaps; }

        /** Get atlas constructed with the sprites from this project */
        ResourceHandle<Image> getSpriteAtlas() const { return m_atlas; }

        /** Get sprite defined as the project icon */
        ResourceHandle<Image> getIcon() const { return m_icon; }

        /** Build atlas out of project sprites */
        void buildAtlas();

        /** Check if project language is JS (ECMAScript) */
        bool isLanguageJS() const { return m_settings.language == "javascript"; }

        /** Check if project language is MSv2 (Microscript v2) */
        bool isLanguageMSv2() const { return m_settings.language == "microscript_v2"; }

        /** Test language as enum value */
        Language getLanguageEnum() const {
            if (isLanguageMSv2()) return L_MicroscriptV2;
            if (isLanguageJS()) return L_JavaScript;
            return L_Unknown;
        }
    };

}
}
