#include "ms_image.hpp"
#include <ms/core/util.hpp>
#include "resources/gpu_texture.hpp"

#include <raylib.h>
#include <memory.h>
#include <rectpack2D/finders_interface.h>

namespace rp = rectpack2D;
using PlatformImage = ::Image;

namespace ms {
namespace res {

    using spaces_type = rp::empty_spaces<false, rp::default_empty_spaces>;
    using rect_type = rp::output_rect_t<spaces_type>;

    struct ImageRect {
        rect_type rect;
        Image const* image;

        ImageRect(Image const* img, const rect_type& rect_) :
            rect(rect_), image(img) { }

        // in-place mutator for rectpack2D
        auto& get_rect() {
            return rect;
        }

        const auto& get_rect() const {
            return rect;
        }
    };

    Image::~Image() {
        PlatformImage* image = reinterpret_cast<PlatformImage*>(m_internalImage);
        if (IsImageValid(*image)) {
            UnloadImage(*image);
            delete image;
            m_internalImage = nullptr;
        }
    }
    
    /** TODO: More thorough error management */
    Resource* Image::loadingHandler(class ms::io::File *file) {
        if (file->status() != io::File::OK)
            return nullptr;

        size_t fsize = file->size();
        unsigned char* fdata = new unsigned char[fsize];
        file->read(reinterpret_cast<void*>(fdata), fsize);
        if (file->status() != io::File::OK) {
            delete[] fdata;
            return nullptr;
        }

        const auto path = file->path();
        const auto pext = path.extension();

        PlatformImage image = LoadImageFromMemory(reinterpret_cast<char const*>(pext.u8string().c_str()), fdata, fsize);
        delete[] fdata;

        if (IsImageValid(image)) {
            PlatformImage* pImage = new PlatformImage;
            memcpy((void*)pImage, &image, sizeof(PlatformImage));

            Image* resImage = new Image(reinterpret_cast<void*>(pImage));
            resImage->m_path = file->path().string();
            return resImage;
        }
        return nullptr;
    }

    size_t Image::getWidth() const {
        PlatformImage& image = *reinterpret_cast<PlatformImage*>(m_internalImage);
        if (IsImageValid(image))
            return image.width;
        return 0;
    }

    size_t Image::getHeight() const {
        PlatformImage& image = *reinterpret_cast<PlatformImage*>(m_internalImage);
        if (IsImageValid(image))
            return image.height;
        return 0;
    }

    ResourceHandle<Image> Image::createAtlas(Image const** images, size_t num, char const* atlasCacheName) {
        int binSide = 256;
        std::vector<ImageRect> rectangles;

        for (int i = 0; i < num; ++i) {
            Image const* image = images[i];
            rectangles.emplace_back(image, rp::rect_xywh(0, 0, image->getWidth(), image->getHeight()));
        }

        bool packingFailed;

        auto successHandler = [](rect_type&) {
            return rp::callback_result::CONTINUE_PACKING;
        };

        auto failHandler = [&packingFailed](rect_type&) {
            packingFailed = true;
            return rp::callback_result::ABORT_PACKING;
        };

        const int discardStep = -4;
        while (true) {
            packingFailed = false;
            const rp::rect_wh resultSize = rp::find_best_packing<spaces_type>(
                rectangles,
                make_finder_input(
                    binSide,
                    discardStep,
                    successHandler,
                    failHandler,
                    rp::flipping_option::DISABLED
                )
            );
            binSide <<= 1;
            DEBUG_ASSERT(binSide <= 2048);

            if (!packingFailed) {
                PlatformImage* pImage = new PlatformImage;
                Image* resImage = new Image(pImage);
                resImage->m_path = atlasCacheName;
                
                PlatformImage bin = GenImageColor(resultSize.w, resultSize.h, ::Color{0,0,0,0});
                for (const auto& rect : rectangles) {
                    const auto& rpRect = rect.get_rect();
                    const Rectangle srcRect = Rectangle{
                        0, 0, 
                        static_cast<float>(rect.image->getWidth()), static_cast<float>(rect.image->getHeight())
                    };
                    const Rectangle dstRect = Rectangle{
                        static_cast<float>(rpRect.x), static_cast<float>(rpRect.y),
                        static_cast<float>(rpRect.w), static_cast<float>(rpRect.h)};
                    ImageDraw(&bin, *(PlatformImage*)(rect.image->getPlatformImage()), srcRect, dstRect, WHITE);

                    // add to image resource atlas rectangles
                    short nframes = static_cast<short>(rect.image->getFrameCount()); // basically: number of vertical splits
                    short fps = static_cast<short>(rect.image->getFPS());
                    resImage->m_atlasRects[rect.image->getPath()] = AtlasRect{
                        rpRect.x, rpRect.y, (short)rpRect.w, (short)rpRect.h, fps, nframes
                    };
                }
                //ExportImage(bin, "./atlas.png"); // debug atlas creation

                // do not unload bin Image, it gets transferred into pImage.
                memcpy((void*)pImage, &bin, sizeof(PlatformImage));
                return ResourceManager::get()->cacheResource<Image>(atlasCacheName, resImage);
            }
        }

        // failed...
        return ResourceHandle<Image>{nullptr};
    }

    ResourceHandle<GPUTexture> Image::toTexture(bool recreate) const {
        ResourceManager* rMan = ResourceManager::get();
        std::string key = m_path + ".gpu";
        
        auto res = rMan->getCached<GPUTexture>(key.c_str());
        if (res.operator->()) {
            if (recreate)
                res->update(this);
            return res;
        }
        
        Texture2D hwTex = LoadTextureFromImage(*(PlatformImage*) getPlatformImage());
        if (IsTextureValid(hwTex)) {
            Texture2D* pTex = new Texture2D(hwTex);
            GPUTexture* gpuTex = new GPUTexture(reinterpret_cast<void*>(pTex));
            return rMan->cacheResource<GPUTexture>(key.c_str(), gpuTex);
        }
        return ResourceHandle<GPUTexture>{nullptr};
    }

    void Image::updateGPUTexture(GPUTexture* hwTex) const {
        auto* pImage = (PlatformImage*) getPlatformImage();
        Texture2D* rlt = reinterpret_cast<Texture2D*>(hwTex->getPlatformTexture());
        UpdateTexture(*rlt, pImage->data);
    }
}
}

