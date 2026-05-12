#include "canvas2d_rc.hpp"

#include <list>
#include <raylib.h>
#include <raymath.h>

namespace ms {
namespace gfx {

    static_assert(sizeof(Color) == sizeof(uint32_t), "Size of color must equate to int color, for bit casting.");

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

    int CanvasRC2D::getWidth() const {
        if (m_window->selectWindow(m_windowId))
            return m_window->getWindowWidth();
        return 0;
    }

    int CanvasRC2D::getHeight() const {
        if (m_window->selectWindow(m_windowId))
            return m_window->getWindowHeight();
        return 0;
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

    void CanvasRC2D::transform(float a, float b, float c, float d, float e, float f) {
        Matrix m3x3 = { a, c, 0.0f, e,
                      b, d, 0.0f, f,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
        m_engine->transform = MatrixMultiply(m_engine->transform, m3x3);
    }

    void CanvasRC2D::translate(float x, float y) {
        transform(0.0, 0.0, 0.0, 0.0, x, y);
    }

    void CanvasRC2D::scale(float w, float h) {
        transform(w, 0.0, 0.0, h, 0.0, 0.0);
    }

    void CanvasRC2D::rotate(float radians) {
        transform(cosf(radians), sinf(radians), -sinf(radians), cosf(radians), 0.0, 0.0);
    }

    void CanvasRC2D::drawQuad(res::GPUTexture* hwTex, float x, float y, float w, float h) {
        drawQuad(hwTex, 0, 0, hwTex->getWidth(), hwTex->getHeight(), x, y, w, h);
    }

    void CanvasRC2D::drawQuad(res::GPUTexture* hwTex, float sx, float sy, float sw, float sh, float x, float y, float w, float h) {
        Rectangle src{ sx, sy, sw, sh };
        Rectangle dst{ x, y, w, h };
        Vector2 origin{ w * .5f, h * .5f }; // microstudio uses the center as the origin
        Texture2D* rlTex = reinterpret_cast<Texture2D*>(hwTex->getPlatformTexture());
        DrawTexturePro(*rlTex, src, dst, origin, 0.f, WHITE);
    }

    void CanvasRC2D::setStrokeColor(uint32_t OxAABBGGRR) {
        m_engine->strokeColor = *(Color*)&OxAABBGGRR;
    }

    void CanvasRC2D::setFillColor(uint32_t OxAABBGGRR) {
        m_engine->fillColor = *(Color*)&OxAABBGGRR;
    }

    void CanvasRC2D::clear() {
        ClearBackground(m_engine->fillColor);
    }

    void CanvasRC2D::clearWithColor(uint32_t OxAABBGGRR) {
        ClearBackground(*(Color*)&OxAABBGGRR);
    }

}
}
