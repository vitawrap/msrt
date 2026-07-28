#include "font_loader.hpp"
#include "io/file.hpp"
#include "platform/platform.hpp"

#ifdef PLATFORM_UNIX
/* as far as I know, fontconfig is the most universal way of finding fonts on nix boxes */
#include <fontconfig/fontconfig.h>
#endif

namespace ms {
namespace gfx {

FontLoader::FontLoader() {
    
}

FontLoader::~FontLoader() {
    
}

bool FontLoader::getPath(std::string_view ftname, std::string& path) {
#ifdef PLATFORM_UNIX
    #ifdef FC_VERSION
        FcConfig* config = FcInitLoadConfigAndFonts();
        FcPattern* pattern = FcNameParse((const FcChar8*)ftname.data());
        FcConfigSubstitute(config, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);
    
        FcResult res;
        FcPattern* font = FcFontMatch(config, pattern, &res);
        if (font) {
            FcChar8* file;
            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
                path = reinterpret_cast<char const*>(file);
                return true;
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pattern);
        return false;
    #else
        /* fontconfig is not on the system (very weird) */
        #warning Fontconfig is not available on this system, only internal fonts will be used.
        return false;
    #endif
#endif
#ifdef PLATFORM_NT
    /* TODO: Regkey based lookup on NT */
    return false;
#endif
#ifdef PLATFORM_UNKNOWN
    return false;
#endif
}

bool FontLoader::load(std::string_view ftname, FontFile& ff) {
    if (m_fontFiles.contains(ftname)) {
        ff = m_fontFiles.find(ftname)->second;
        return true;
    } else {
        std::string path;
        if (getPath(ftname, path)) {
            auto file = io::FileSystem::openForRead(path.c_str());
            if (file) {
                ff.size = file->size();
                ff.data = new uint8_t[ff.size];
                file->read(ff.data, ff.size);
                m_fontFiles.emplace(ftname, ff);
                return true;
            }
            return false;
        }
        return false;
    }
}

FontLoader* FontLoader::get() {
    static FontLoader instance;
    return &instance;
}

}
}
