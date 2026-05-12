#pragma once

#include "resource.hpp"

namespace ms {
namespace res {

    /**
     * @brief GPU texture resource
     * A GPU texture resource cannot be loaded from disk,
     * it can only be created at runtime, whether procedurally or from an image.
     */
    class GPUTexture : public Resource {
        friend class Image;

        void* m_pHwTex;

        GPUTexture(void* pHwTex);

    public:
        ~GPUTexture();

        /** Update texture from image */
        void update(const class Image* from);
        
        /** Get ID allocated by the renderer */
        unsigned int getHardwareID() const;
        int getFormatID() const;

        int getWidth() const;
        int getHeight() const;
        int getMipmapCount() const;

        bool isValid() const;

        void* getPlatformTexture() const { return m_pHwTex; }
    };

}
}

