#include "project.hpp"
#include "io/file.hpp"
#include "io/json.hpp"
#include "io/log.hpp"

namespace ms {
namespace res {

    Project::SpriteSheet const* Project::findSheetInfo(std::string_view path) const {
        auto& sheets = m_settings.spritesheets;
        auto ret = std::find_if(sheets.begin(), sheets.end(), [path](SpriteSheet const& v) {
            return v.filename == path;
        });
        if (ret == sheets.cend())
            return nullptr;
        return ret.base();
    }

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
                    m_settings.title        = (char const*) root["title"];
                    m_settings.aspect       = (char const*) root["aspect"];
                    m_settings.orientation  = (char const*) root["orientation"];
                    m_settings.language     = (char const*) root["language"];

                    auto files = root["files"]; io::CJSONNode file;
                    CJSONNode_forEach(file, files) {
                        std::filesystem::path key(file->string);
                        if (key.extension() == ".png") {
                            io::CJSONNode props;
                            CJSONNode_forEach(props, file) {
                                if (strcmp(props->string, "properties") == 0) {
                                    bool isSheet = false;
                                    SpriteSheet sinfo;

                                    io::CJSONNode prop;
                                    CJSONNode_forEach(prop, props) {
                                        if (strcmp(prop->string, "frames") == 0) {
                                            if ((sinfo.nframes = prop->valueint) > 1)
                                                isSheet = true;
                                        }
                                        if (strcmp(prop->string, "fps") == 0)
                                            sinfo.nfps = prop->valueint;
                                    }
        
                                    if (isSheet) {
                                        sinfo.filename = key;
                                        m_settings.spritesheets.push_back(sinfo);
                                    }
                                }
                            }
                        }
                    }
                    
                } catch (io::CJSONError& jsErr) {
                    LOG_MSGF("JSON Error: %s when reading out project.json for \"%s\".\n", jsErr.what(), filename);
                    return false;
                }
            }

            // get asset files... (scripts, sprites, music, sounds)
            std::vector<std::string> paths;
            m_files.getFileList(paths);
            for (const auto& path : paths) {
                bool cache_asset = false;
                if ((path.compare(0, 3, "ms/") == 0)
                || (path.compare(0, 6, "music/") == 0)
                || (path.compare(0, 7, "sounds/") == 0)
                || (path.compare(0, 8, "sprites/") == 0)) {
                    cache_asset = true;
                }
                std::filesystem::path filename(path);

                // for languages other than microscript, remap their ext to .ms as expected by player.
                if (filename.extension() == ".js") {
                    filename.replace_extension(".ms");
                }

                // directory entries have no ext, check if we caught a directory
                if (cache_asset && filename.has_extension()) {
                    size_t fileSize = 0;
                    auto bufHandle = m_files.readFile(path.c_str(), fileSize);
                    io::MemFile file(filename.c_str(), bufHandle.get(), fileSize);
                    auto res = ResourceManager::get()->loadResourceOpaque(&file);

                    // if it's a script file, track it
                    if (filename.extension() == ".ms") {
                        m_scripts.emplace(filename, res.as<Script>());
                        LOG_MSGF("Adding script %s\n", filename.c_str());
                    }
                    // and also if it's a sprite (track for atlasing)
                    else if (path.compare(0, 8, "sprites/") == 0) {
                        m_sprites.emplace(filename, res.as<Image>());
                        LOG_MSGF("Adding sprite %s\n", filename.c_str());

                        // can we get spritesheet info from it?
                        if (SpriteSheet const* sinfo = findSheetInfo(filename.string()))
                            res.as<Image>()->setVSheetInfo(sinfo->nframes, sinfo->nfps);

                        // keep reference to icon
                        if (path == "sprites/icon.png")
                            m_icon = res.as<Image>();
                    }
                }
            }

            return true;
        }
        return false;
    }

    void Project::buildAtlas() {
        std::vector<const Image*> imagePtrs;
        for (const auto [_, imageRef] : m_sprites)
            imagePtrs.push_back(imageRef.operator->());
        
        m_atlas = Image::createAtlas(imagePtrs.data(), imagePtrs.size(), "p_atlas");
    }

    void Project::close()
    {
        m_files.close();
    }
}
}
