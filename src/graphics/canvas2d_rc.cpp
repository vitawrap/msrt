#include "canvas2d_rc.hpp"

#include <raylib.h>

namespace ms {
namespace gfx {

    /**
     * @brief This holds all of the raylib-specific data not exposed in class header
     */
    struct CanvasEngine {
        Camera2D camera;
    };

    CanvasRC2D::CanvasRC2D() :
        m_window(nullptr),
        m_windowId(0),
        m_engine(nullptr)
    {}

    void CanvasRC2D::init(platform::IWindowManager* wm, int wid) {
        free();
        if ((wm->getWindowCount() > wid) && wm->selectWindow(wid)) {
            m_window = wm;
            m_windowId = wid;
            m_engine = new CanvasEngine;

            
        }
    }

    void CanvasRC2D::free() {
        if (m_engine) {
            delete m_engine;
            m_engine = nullptr;
        }
    }

    void CanvasRC2D::beginFrame() {
        BeginDrawing();
    }

    void CanvasRC2D::submitFrame() {
        EndDrawing();
    }

}
}
