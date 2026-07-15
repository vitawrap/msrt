#include "canvas2d_rc.hpp"
#include "graphics/font_loader.hpp"
#include <ms/core/view_map.hpp>

#include <stdio.h>

#include <list>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

FONT_RESOLVE_EMBED(bitcell_ttf)
#define MSFONT_DEFNAME "BitCell"

STATIC_RESOLVE_EMBED(__shader_basic2d_vert)
STATIC_RESOLVE_EMBED(__shader_basic2d_frag)

namespace ms {
namespace gfx {

    static_assert(sizeof(Color) == sizeof(uint32_t), "Size of color must equate to int color, for bit casting.");

    struct CanvasFont {
        Font font;

        ~CanvasFont() {
            if (IsFontValid(font)) UnloadFont(font);
        }

        CanvasFont(Font&& font):
            font(std::move(font))
        {}
    };

    struct CanvasState {
        double lineWidth;
        Matrix transform;
        Color fillColor;
        Color strokeColor;
        std::string fontName;
    };

    struct CanvasMesh {
        Mesh data;

        CanvasMesh(unsigned triCount)
            : data{}
        {
            data.vertexCount = triCount * 3;
            data.triangleCount = triCount;
            data.vertices = (float*) MemAlloc(data.vertexCount * 3 * sizeof(float));
            data.texcoords = (float*) MemAlloc(data.vertexCount * 2 * sizeof(float));
            data.normals = nullptr;
        }

        ~CanvasMesh() {
            UnloadMesh(data);
        }
    };

    /**
     * @brief This holds all of the raylib-specific data not exposed in class header
     */
    struct CanvasEngine {
        Shader shader;
        RenderTexture renderTexture;
        Matrix transform; // camera transform
        Color fillColor;
        Color strokeColor;
        float lineWidth;
        CanvasFont* font;
        std::string fontName;

        struct {
            int locationUVOffset;
        } uniform;

        std::list<CanvasState> states;
        ms::unordered_map<CanvasFont> loadedFonts;
    };

    CanvasRC2D::CanvasRC2D() :
        m_window(nullptr),
        m_windowId(0),
        m_engine(nullptr),
        m_drawAnchorX(0.5),
        m_drawAnchorY(0.5),
        m_drawing(false)
    {}

    void CanvasRC2D::init(platform::IWindowManager* wm, int wid) {
        free();
        if ((wm->getWindowCount() > wid) && wm->selectWindow(wid)) {
            m_window = wm;
            m_windowId = wid;
            m_width = wm->getWindowWidth();
            m_height = wm->getWindowHeight();
            m_engine = new CanvasEngine{};

            m_engine->transform = MatrixIdentity();
            m_engine->strokeColor = BLACK;
            m_engine->fillColor = BLACK;
            m_engine->lineWidth = 1.f;
            m_engine->font = nullptr;
            m_engine->renderTexture = LoadRenderTexture(m_width, m_height);
            m_engine->shader = LoadShaderFromMemory(__shader_basic2d_vert, __shader_basic2d_frag);

            // pull the uniforms we care about
            if (IsShaderValid(m_engine->shader)) {
                float defaultUVOffset = 0.f;
                m_engine->uniform.locationUVOffset = GetShaderLocation(m_engine->shader, "uvOffset");
                SetShaderValue(m_engine->shader, m_engine->uniform.locationUVOffset, &defaultUVOffset, SHADER_UNIFORM_FLOAT);
            }
            
            rlSetLineWidth(m_engine->lineWidth);
            rlEnableSmoothLines();
        }
    }

    void CanvasRC2D::free() {
        if (m_engine) {
            if (IsRenderTextureValid(m_engine->renderTexture))
                UnloadRenderTexture(m_engine->renderTexture);
            if (IsShaderValid(m_engine->shader))
                UnloadShader(m_engine->shader);
            delete m_engine;
            m_engine = nullptr;
        }
    }

    void CanvasRC2D::setFont(const char* fontFaceName) {
        m_engine->fontName = fontFaceName;
        m_engine->font = nullptr;
    }

    /**
     * validateFont is the only mutator of the loadedFonts map, so it is the only function that
     * can assign a direct ptr into the loadedFonts map, knowing it will not get invalidated.
     * (Unless other methods mutating the map are careful enough to set m_engine->font to null!)
     */
    bool CanvasRC2D::validateFont(int ftSize) {
        if (!m_engine->font) {
            if (m_engine->fontName.empty()) failsafe_default_font:
                m_engine->fontName = MSFONT_DEFNAME;

            char ftKey[64];
            snprintf(ftKey, 64, "%s:%d", m_engine->fontName.c_str(), ftSize);

            auto& ftCache = m_engine->loadedFonts;
            if (ftCache.contains(ftKey)) { // not as cheap as .find, but avoids a try catch...
                m_engine->font = &(ftCache.at(ftKey));
                return true;
            } else {
                /** TODO: move away from loading specific fonts hardcoded here */
                if (m_engine->fontName == MSFONT_DEFNAME) {
                    Font font = LoadFontFromMemory(".ttf", reinterpret_cast<const unsigned char*>(embed::__font_bitcell_ttf),
                    embed::__font_bitcell_ttf_size, ftSize, nullptr, 0);
                    if (IsFontValid(font))
                        ftCache.emplace(ftKey, std::move(font));
                } else {
                    std::string path;
                    FontLoader::FontFile ff;
                    auto* sysFonts = FontLoader::get();
                    if (sysFonts->load(m_engine->fontName, ff)) {
                        Font font = LoadFontFromMemory(".ttf", reinterpret_cast<const unsigned char*>(ff.data),
                        ff.size, ftSize, nullptr, 0);
                        if (IsFontValid(font))
                            ftCache.emplace(ftKey, std::move(font));
                    }
                }

                // try one last time with the fonts we just loaded
                if (ftCache.contains(ftKey)) {
                    m_engine->font = &(ftCache.at(ftKey));
                    return true;
                }

                // still nothing? do a roundtrip with the default font...
                if (m_engine->fontName != MSFONT_DEFNAME)
                    goto failsafe_default_font;
            }
            return false;
        }
        return true;
    }

    void CanvasRC2D::push() {
        m_engine->states.emplace_back();
        CanvasState& state = m_engine->states.back();
        state.transform     = m_engine->transform;
        state.fillColor     = m_engine->fillColor;
        state.strokeColor   = m_engine->strokeColor;
        state.lineWidth     = m_engine->lineWidth;
        state.fontName      = m_engine->fontName;
    }

    bool CanvasRC2D::pop() {
        if (m_engine->states.empty())
            return false;
        CanvasState& state = m_engine->states.back();
        m_engine->transform     = state.transform;
        m_engine->fillColor     = state.fillColor;
        m_engine->strokeColor   = state.strokeColor;
        m_engine->lineWidth     = state.lineWidth;
        m_engine->fontName      = state.fontName;
        m_engine->font          = nullptr;
        m_engine->states.pop_back();
        rlSetLineWidth(state.lineWidth);
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
        //m_canvas->fillRect(x - (w * .5f), -y - (h * .5f), w, h);
        DrawRectangle(x - (w * m_drawAnchorX), (-h * (1.f - m_drawAnchorY)) - y, w, h, m_engine->fillColor);
    }

    void CanvasRC2D::strokeRect(int x, int y, int w, int h) {
        DrawRectangleLines(x - (w * m_drawAnchorX), (-h * (1.f - m_drawAnchorY)) - y, w, h, m_engine->strokeColor);
    }

    void CanvasRC2D::strokeRound(float x, float y, float w, float h) {
        float realAnchorX = Remap(m_drawAnchorX, 0.f, 1.f, -.5f, .5f);
        float realAnchorY = Remap(m_drawAnchorY, 0.f, 1.f, -.5f, .5f);
        DrawEllipseLines(x - (w * realAnchorX), (h * (realAnchorY)) - y, w * .5f, h * .5f, m_engine->fillColor);
    }

    void CanvasRC2D::fillRound(float x, float y, float w, float h) {
        float realAnchorX = Remap(m_drawAnchorX, 0.f, 1.f, -.5f, .5f);
        float realAnchorY = Remap(m_drawAnchorY, 0.f, 1.f, -.5f, .5f);
        DrawEllipse(x - (w * realAnchorX), (h * (realAnchorY)) - y, w * .5f, h * .5f, m_engine->strokeColor);
    }

    void CanvasRC2D::setUVOffsetY(float offset) {
        if (IsShaderValid(m_engine->shader) && m_engine->uniform.locationUVOffset != -1)
            SetShaderValue(m_engine->shader, m_engine->uniform.locationUVOffset, &offset, SHADER_UNIFORM_FLOAT);
    }

    void CanvasRC2D::beginFrame() {
        BeginTextureMode(m_engine->renderTexture);

        // have to use rlgl here, because Camera2D is weirdly restrictive.
        rlLoadIdentity();
        rlMultMatrixf(MatrixToFloat(m_engine->transform));
        m_drawing = true;

        ClearBackground(BLACK); // default microscript clear color is black
    }

    void CanvasRC2D::submitFrame() {
        EndTextureMode();
        // render RT on screen
        m_window->selectWindow(m_windowId);
        Rectangle src{ 0, 0, m_width, -m_height };
        Rectangle dst{ 0, 0, (float)m_window->getWindowWidth(), (float)m_window->getWindowHeight()};
        BeginDrawing();
        BeginShaderMode(m_engine->shader);
        
        DrawTexturePro(m_engine->renderTexture.texture, src, dst, Vector2{0,0}, 0.f, WHITE);
        
        // draw an FPS counter over everything
        char fpsText[64];
        snprintf(fpsText, 64, "%d FPS", GetFPS());
        DrawText(fpsText, 8, 8, 20, WHITE);
        
        EndShaderMode();
        EndDrawing();
        m_drawing = false;
    }

    platform::IWindowManager* CanvasRC2D::getWindowManager() const {
        return m_window;
    }

    int CanvasRC2D::getWindowID() const {
        return m_windowId;
    }

    void CanvasRC2D::resize(float w, float h) {
        m_width = w;
        m_height = h;
        if (IsRenderTextureValid(m_engine->renderTexture))
            UnloadRenderTexture(m_engine->renderTexture);
        m_engine->renderTexture = LoadRenderTexture(w, h);
    }

    void CanvasRC2D::transform(float a, float b, float c, float d, float e, float f) {
        Matrix m3x3 = { a, c, 0.0f, e,
                      b, d, 0.0f, f,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
        m_engine->transform = MatrixMultiply(m_engine->transform, m3x3);
        /** TODO: this is a workaround, m_engine->transform could be desynced from gl matrix. */
        if (m_drawing) rlMultMatrixf(MatrixToFloat(m3x3));
    }

    void CanvasRC2D::translate(float x, float y) {
        transform(1.0, 0.0, 0.0, 1.0, x, y);
    }

    void CanvasRC2D::scale(float w, float h) {
        transform(w, 0.0, 0.0, h, 0.0, 0.0);
    }

    void CanvasRC2D::rotate(float radians) {
        transform(cosf(radians), sinf(radians), -sinf(radians), cosf(radians), 0.0, 0.0);
    }

    void CanvasRC2D::setLineWidth(float w) {
        m_engine->lineWidth = w;
        rlSetLineWidth(w);
    }

    void CanvasRC2D::drawLine(float x0, float y0, float x1, float y1) {
        Vector2 begin {x0, -y0};
        Vector2 end {x1, -y1};
        DrawLineEx(begin, end, m_engine->lineWidth, m_engine->strokeColor);
    }

    void CanvasRC2D::drawQuad(res::GPUTexture* hwTex, float x, float y, float w, float h) {
        drawQuad(hwTex, 0, 0, hwTex->getWidth(), hwTex->getHeight(), x, y, w, h);
    }

    void CanvasRC2D::drawQuad(res::GPUTexture* hwTex, float sx, float sy, float sw, float sh, float x, float y, float w, float h) {
        Rectangle src{ sx, sy, sw, sh };
        Rectangle dst{ x, y, w, h };
        Vector2 origin{ w * m_drawAnchorX, h - (h * m_drawAnchorY) }; // microstudio uses the center as the origin
        Texture2D* rlTex = reinterpret_cast<Texture2D*>(hwTex->getPlatformTexture());
        Color whiteAlpha { 255, 255, 255, m_engine->fillColor.a };
        // strangely, microstudio doesn't use the effective drawing color for sprites, just the alpha.
        DrawTexturePro(*rlTex, src, dst, origin, 0.f, whiteAlpha);
    }

    void CanvasRC2D::drawText(std::string_view text, float x, float y, int ftSize, float deblurFactor) {
        if (validateFont(ftSize * deblurFactor)) {
            Font& ft = m_engine->font->font;
        
            Vector2 textSz = MeasureTextEx(ft, text.data(), ftSize, 0);
            
            //Vector2 origin{ (textSz.x * -m_drawAnchorX) + x, (textSz.y * (1.f - m_drawAnchorY)) - y };
            Vector2 origin{ (textSz.x * -m_drawAnchorX) + x, (textSz.y * -.5f) - y };
            DrawTextEx(m_engine->font->font, text.data(), origin, ftSize, 0, m_engine->fillColor);
        }
    }

    void CanvasRC2D::setStrokeColor(uint32_t OxAABBGGRR) {
        m_engine->strokeColor = *(Color*)&OxAABBGGRR;
    }

    void CanvasRC2D::setFillColor(uint32_t OxAABBGGRR) {
        m_engine->fillColor = *(Color*)&OxAABBGGRR;
    }

    void CanvasRC2D::setDrawAnchors(float ratioX, float ratioY) {
        m_drawAnchorX = ratioX;
        m_drawAnchorY = ratioY;
    }

    void CanvasRC2D::clear() {
        ClearBackground(m_engine->fillColor);
    }

    void CanvasRC2D::clearWithColor(uint32_t OxAABBGGRR) {
        ClearBackground(*(Color*)&OxAABBGGRR);
    }

    std::weak_ptr<CanvasMesh> CanvasRC2D::beginMesh(unsigned triCount) {
        auto cm = std::make_shared<CanvasMesh>(triCount);
        m_meshes.push_back(std::move(cm));
        return std::weak_ptr<CanvasMesh>(m_meshes.back());
    }

    void CanvasRC2D::setTriangle(std::weak_ptr<CanvasMesh> mesh, unsigned index, CanvasTriangle const& tri) {
        auto cm = mesh.lock();
        if (!cm || index >= cm->data.triangleCount) return;
        
        cm->data.vertices[index * 9 + 0] = tri.x0;
        cm->data.vertices[index * 9 + 1] = tri.y0;
        cm->data.vertices[index * 9 + 2] = 0.f;
        cm->data.vertices[index * 9 + 3] = tri.x1;
        cm->data.vertices[index * 9 + 4] = tri.y1;
        cm->data.vertices[index * 9 + 5] = 0.f;
        cm->data.vertices[index * 9 + 6] = tri.x2;
        cm->data.vertices[index * 9 + 7] = tri.y2;
        cm->data.vertices[index * 9 + 8] = 0.f;

        cm->data.texcoords[index * 6 + 0] = tri.u0;
        cm->data.texcoords[index * 6 + 1] = tri.v0;
        cm->data.texcoords[index * 6 + 2] = tri.u1;
        cm->data.texcoords[index * 6 + 3] = tri.v1;
        cm->data.texcoords[index * 6 + 4] = tri.u2;
        cm->data.texcoords[index * 6 + 5] = tri.v2;
    }

    void CanvasRC2D::endMesh(std::weak_ptr<CanvasMesh> mesh) {
        auto cm = mesh.lock();
        if (cm) UploadMesh(&cm->data, false);
    }
}
}
