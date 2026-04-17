#include "project.hpp"
#include "io/file.hpp"
#include "io/json.hpp"
#include "io/log.hpp"
#include "resource_manager.hpp"

namespace ms {
namespace res {

    bool Project::open(char const *filename)
    {
        close();

        if (m_files.open(filename)) {
            
            // read out settings from project
            if (m_files.fileExists("project.json")) {
                size_t textsize;
                auto textbuf = m_files.readFile("project.json", textsize);
                std::string text(textbuf.get(), textsize);
                if (!textbuf) return false;
                try {
                    io::CJSON json;
                    auto root = json.parse(text.c_str());
                    m_settings.title = (char const*) root["title"];
                    
                } catch (io::CJSONError& jsErr) {
                    return false;
                }
            }

            // get asset files... (sprites, music, sounds)
            std::vector<std::string> paths;
            m_files.getFileList(paths);
            for (const auto& path : paths) {
                bool cache_asset = false;
                if ((path.compare(0, 6, "music/") == 0)
                || (path.compare(0, 7, "sounds/") == 0)
                || (path.compare(0, 8, "sprites/") == 0)) {
                    cache_asset = true;
                }
                // directory entries have no ext, check if we caught a directory
                if (cache_asset && (path.rfind('.') != path.npos)) {
                    size_t fileSize = 0;
                    auto bufHandle = m_files.readFile(path.c_str(), fileSize);
                    io::MemFile file(path.c_str(), bufHandle.get(), fileSize);
                    //auto res = ResourceManager::get()->loadResourceOpaque(&file);
                }
            }

            return true;
        }
        return false;
    }

    void Project::close()
    {
        m_files.close();
    }
}
}
