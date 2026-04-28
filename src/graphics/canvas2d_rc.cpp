#include "canvas2d_rc.hpp"

#include <list>
#include <raylib.h>
#include <raymath.h>

namespace ms {
namespace gfx {

    struct CanvasState {
        double lineWidth;
        Matrix transform;
        Color fillColor;
        Color strokeColor;
    };

    /**
     * @brief This holds all of the raylib-specific data not exposed in class header
     */
    struct CanvasEngine {
        Matrix transform; // camera transform
        Color fillColor;
        Color strokeColor; 

        std::list<CanvasState> states;
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
            m_engine = new CanvasEngine{};

            m_engine->transform = MatrixIdentity();
            m_engine->strokeColor = BLACK;
            m_engine->fillColor = BLACK;
        }
    }

    void CanvasRC2D::free() {
        if (m_engine) {
            delete m_engine;
            m_engine = nullptr;
        }
    }

    void CanvasRC2D::push() {
        m_engine->states.emplace_back();
        CanvasState& state = m_engine->states.back();
        state.transform     = m_engine->transform;
        state.fillColor     = m_engine->fillColor;
        state.strokeColor   = m_engine->strokeColor;
    }

    bool CanvasRC2D::pop() {
        if (m_engine->states.empty())
            return false;
        CanvasState& state = m_engine->states.back();
        m_engine->transform     = state.transform;
        m_engine->fillColor     = state.fillColor;
        m_engine->strokeColor   = state.strokeColor;
        m_engine->states.pop_back();
        return true;
    }

    void CanvasRC2D::fillRect(int x, int y, int w, int h) {
        DrawRectangle(x, y, w, h, m_engine->fillColor);
    }

    void CanvasRC2D::strokeRect(int x, int y, int w, int h) {
        DrawRectangleLines(x, y, w, h, m_engine->strokeColor);
    }

    void CanvasRC2D::beginFrame() {
        BeginDrawing();
        ClearBackground(BLACK); // default microscript clear color is black
    }

    void CanvasRC2D::submitFrame() {
        EndDrawing();
    }

}
}
