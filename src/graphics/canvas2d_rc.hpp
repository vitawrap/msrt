#pragma once

#include "canvas_rc.hpp"

namespace ms {
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

        void beginFrame() override;
        void submitFrame() override;
    };

}
}
