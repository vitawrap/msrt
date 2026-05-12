#include "gpu_texture.hpp"

#include "ms_image.hpp"
#include <raylib.h>

namespace ms {
namespace res {

    GPUTexture::GPUTexture(void* pHwTex):
        m_pHwTex(pHwTex)
    {
    }

    GPUTexture::~GPUTexture() {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        if (IsTextureValid(*rlt))
            UnloadTexture(*rlt);
        delete rlt;
        m_pHwTex = nullptr;
    }

    void GPUTexture::update(const Image* from) {
        from->updateGPUTexture(this);
    }

    unsigned int GPUTexture::getHardwareID() const {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        return rlt->id;
    }

    int GPUTexture::getFormatID() const {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        return rlt->format;
    }

    int GPUTexture::getWidth() const {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        return rlt->width;
    }

    int GPUTexture::getHeight() const {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        return rlt->height;
    }

    int GPUTexture::getMipmapCount() const {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        return rlt->mipmaps;
    }

    bool GPUTexture::isValid() const {
        Texture2D* rlt = reinterpret_cast<Texture2D*>(m_pHwTex);
        return rlt->id != 0;
    }
}
}
