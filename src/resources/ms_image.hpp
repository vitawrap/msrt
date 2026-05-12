#pragma once

#include "resources/resource.hpp"
#include "resource_manager.hpp"
#include "core/robin_hood.hpp"
#include "io/file.hpp"

namespace ms {
namespace res {

    class Image : public Resource {
    public:
        struct AtlasRect {
            int x, y, width, height;
        };

    protected:
        robin_hood::unordered_map<
            std::string,
            AtlasRect
        > m_atlasRects;

        std::string m_path;
        void* m_internalImage;

        Image(void* image):
            m_internalImage(image)
        {}
    public:
        ~Image();

        /** Get path inside project archive of script */
        std::string const& getPath() const { return m_path; }

        /** Get ptr to image data as defined by platform library */
        void* getPlatformImage() const { return m_internalImage; }

        size_t getWidth() const;
        size_t getHeight() const;

        /** Create and cache a new image resource from multiple images, visually containing all of them */
        static ResourceHandle<Image> createAtlas(Image const** images, size_t num, char const* atlasCacheName);

        bool isAtlas() const { return m_atlasRects.size(); }
        bool findAtlasRect(std::string const& name, AtlasRect& outRect) {
            auto itr = m_atlasRects.find(name);
            if (itr != m_atlasRects.end()) {
                outRect = itr->second;
                return true;
            }
            return false;
        }

        DECLARE_LOADER;
    };

}
}
