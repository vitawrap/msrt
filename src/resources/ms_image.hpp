#pragma once

#include "resources/resource.hpp"
#include "io/file.hpp"

namespace ms {
namespace res {

    class Image : public Resource {
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

        DECLARE_LOADER;
    };

}
}
