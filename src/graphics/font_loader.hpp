#pragma once

#include <ms/core/view_map.hpp>
#include <string_view>
#include <string>

namespace ms {
namespace gfx {

    class FontLoader {
    public:
        struct FontFile {
            void* data;
            size_t size;
        };
    private:
        ms::unordered_map<FontFile> m_fontFiles;
        
        static bool getPath(std::string_view ftname, std::string& path);
    
    public:
        FontLoader();
        ~FontLoader();

        bool load(std::string_view ftname, FontFile& file);

        static FontLoader* get();
    };
    
}
}
