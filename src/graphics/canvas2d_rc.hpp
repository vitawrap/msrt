#pragma once

#include <stdint.h>
#include "canvas_rc.hpp"
#include "resources/gpu_texture.hpp"

namespace ms {
namespace res {
    class GPUTexture;
}

namespace gfx {

    struct CanvasEngine;

    class CanvasRC2D : public CanvasRC {
        platform::IWindowManager* m_window;
        int m_windowId;

        /** Renderer-specific storage */
        CanvasEngine* m_engine;

    public:
        CanvasRC2D();
        ~CanvasRC2D() { free(); }

        bool isReady() const override { return m_engine; }
        void init(platform::IWindowManager *wm, int wid = 0) override;
        void free() override;

        void push() override;
        bool pop() override;

        int getWidth() const override;
        int getHeight() const override;

        void beginFrame() override;
        void submitFrame() override;

        /* Canvas2D operations */

        void fillRect(int x, int y, int w, int h);
        void strokeRect(int x, int y, int w, int h);
        void transform(float a, float b, float c, float d, float e, float f); // 3x2 matrix
        void translate(float x, float y);
        void rotate(float radians);
        void scale(float w, float h);
        void setStrokeColor(uint32_t OxAABBGGRR);
        void setFillColor(uint32_t OxAABBGGRR);
        void clearWithColor(uint32_t OxAABBGGRR);
        void clear();
        void drawQuad(res::GPUTexture* hwTex, float x, float y, float w, float h);
        void drawQuad(res::GPUTexture* hwTex, float sx, float sy, float sw, float sh, float x, float y, float w, float h);
    };

}
}
