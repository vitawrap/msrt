#include "ms_image.hpp"

#include <raylib.h>
#include <memory.h>

using PlatformImage = ::Image;

namespace ms {
namespace res {

    Image::~Image() {
        PlatformImage& image = *reinterpret_cast<PlatformImage*>(m_internalImage);
        if (IsImageValid(image)) {
            UnloadImage(image);
            delete &image;
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

        PlatformImage image = LoadImageFromMemory(pext.c_str(), fdata, fsize);
        delete[] fdata;

        if (IsImageValid(image)) {
            PlatformImage* pImage = new PlatformImage;
            memcpy((void*)pImage, &image, sizeof(PlatformImage));

            Image* resImage = new Image(reinterpret_cast<void*>(pImage));
            resImage->m_path = file->path();
            return resImage;
        }
        return nullptr;
    }
}
}

