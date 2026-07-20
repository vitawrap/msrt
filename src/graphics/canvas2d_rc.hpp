#pragma once

#include <memory>
#include <stdint.h>
#include "canvas_rc.hpp"
#include "resources/gpu_texture.hpp"

namespace ms {
namespace res {
    class GPUTexture;
}

namespace gfx {

    struct CanvasEngine;
    struct CanvasMesh;
    struct CanvasDynamicMesh;

    struct CanvasTriangle {
        float x0, y0, u0, v0;
        float x1, y1, u1, v1;
        float x2, y2, u2, v2;
    };

    class CanvasRC2D : public CanvasRC {
        platform::IWindowManager* m_window;
        int m_windowId;
        bool m_drawing;

        /** Renderer-specific storage */
        CanvasEngine* m_engine;
        
        /** Virtual render dimensions */
        float m_width, m_height;

        /** Drawing anchor */
        float m_drawAnchorX, m_drawAnchorY;

        bool validateFont(int ftSize);

        /** Allocated meshes */
        std::vector<std::shared_ptr<CanvasMesh>> m_meshes;

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

        platform::IWindowManager* getWindowManager() const override;
        int getWindowID() const override;

        void beginFrame() override;
        void submitFrame() override;

        void resize(float w, float h) override;

        /* Canvas2D operations */

        void fillRect(int x, int y, int w, int h);
        void strokeRect(int x, int y, int w, int h);
        void fillRound(float x, float y, float w, float h);
        void strokeRound(float x, float y, float w, float h);
        void drawLine(float x0, float y0, float x1, float y1);
        void transform(float a, float b, float c, float d, float e, float f); // 3x2 matrix
        void translate(float x, float y);
        void setLineWidth(float w);
        void rotate(float radians);
        void scale(float w, float h);
        void setStrokeColor(uint32_t OxAABBGGRR);
        void setFillColor(uint32_t OxAABBGGRR);
        void clearWithColor(uint32_t OxAABBGGRR);
        void clear();
        void setDrawAnchors(float ratioX, float ratioY);
        void drawQuad(res::GPUTexture* hwTex, float x, float y, float w, float h);
        void drawQuad(res::GPUTexture* hwTex, float sx, float sy, float sw, float sh, float x, float y, float w, float h);
        void drawText(std::string_view text, float x, float y, int ftSize, float deblurFactor = 1.f);
        void setFont(char const* fontFaceName);
        void setUVOffsetY(float offset);

        /* Canvas2D lower level drawing ops */

        std::weak_ptr<CanvasMesh> beginMesh(unsigned triCount);
        void endMesh(std::weak_ptr<CanvasMesh> mesh);

        std::weak_ptr<CanvasDynamicMesh> beginDynamicMesh(unsigned triReserve = 0, unsigned pageSize = 2048);
        void endDynamicMesh(std::weak_ptr<CanvasDynamicMesh> mesh, bool compact = false);

        static void addTriangle(std::weak_ptr<CanvasMesh> mesh, CanvasTriangle const& tri);
        static void addTriangle(std::weak_ptr<CanvasDynamicMesh> mesh, CanvasTriangle const& tri);
    };

}
}

/* Retrieve a font entry in script_embed.S */
#define FONT_RESOLVE_EMBED(global_name) STATIC_RESOLVE_EMBED(__font_##global_name)
