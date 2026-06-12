#include "screen.hpp"
#include "core/app.hpp"
#include <algorithm>
#include <string_view>

namespace ms {
namespace browser {

    constexpr float deg2rad = 0.017453292519943298f;

    Screen::Screen() :
        m_runtime(nullptr),
        m_canvas(nullptr),
        m_transX(0.f),
        m_transY(0.f),
        m_scaleX(1.f),
        m_scaleY(1.f),
        m_degrees(0.f),
        m_objectDegrees(0.f),
        m_objectScaleX(1.f),
        m_objectScaleY(1.f),
        m_anchorX(0.f),
        m_anchorY(0.f),
        m_lineWidth(1.f),
        m_ratio(1.f),
        m_screenTransform(false)
    {}

    void Screen::initContext() {
        if (m_canvas) {
            m_canvas->pop();
        } else {
            auto* app = Application::get();
            m_canvas = app->getBrowserContext()->getCanvas();
            if (!m_canvas->isReady())
                m_canvas->init(app->getWindowManager());
        }
        if (!m_canvas->isReady()) return;
        m_canvas->push();
        float ratio = std::min(m_canvas->getWidth() / 200, m_canvas->getHeight() / 200);
        m_canvas->scale(ratio, ratio);
        m_canvas->translate(m_canvas->getWidth() * .5f, m_canvas->getHeight() * .5f);
        m_width = m_canvas->getWidth() / ratio;
        m_height = m_canvas->getHeight() / ratio;
        m_ratio = ratio;

        // keep reference to main sprite atlas
        auto* project = Application::get()->getProject();
        m_atlas = project->getSpriteAtlas();
        DEBUG_ASSERT(m_atlas.operator->() && m_atlas->isAtlas());
    }

    void Screen::resize() {
        auto* wm = Application::get()->getWindowManager();
        wm->selectWindow(0);
        float cw = wm->getWindowWidth();
        float ch = wm->getWindowHeight();
        float ratio = 1.f, w = 640, h = 480;
        auto arType = m_runtime->getAspectRatio();
        float arValue = Runtime::getRatioFor(arType);
        bool min = arType > Runtime::AR_MinValues;
        if (arType != Runtime::AR_Unknown) {
            if (min) {
                switch (m_runtime->getOrientation()) {
                default_orient_min:
                    if (ch > cw) {
                    case Runtime::Portrait:
                        ratio = std::max(ratio, ch / cw);
                        break;
                    } else {
                    case Runtime::Landscape:
                        ratio = std::max(ratio, cw / ch);
                        break;
                    }
                    default:
                        goto default_orient_min;
                }
            }
            float r = 0.f;
            switch (m_runtime->getOrientation()) {
            default_orient:
                if (cw > ch) {
                case Runtime::Portrait:
                    r = std::min(cw / ratio, ch) / ch;
                    w = ch * r * ratio;
                    h = ch * r;
                    break;
                } else {
                case Runtime::Landscape:
                    r = std::min(cw, ch / ratio) / cw;
                    w = cw * r;
                    h = cw * r * ratio;
                    break;
                }
                default:
                    goto default_orient;
            }
        } else {
            w = cw;
            h = ch;
        }
        wm->setWindowSize(w, h, false); // we might come from a window event
        if (m_canvas && m_canvas->isReady())
            m_canvas->resize(w * ratio, h * ratio);
        initContext();
    }

    void Screen::startControl() {
        
    }

    void Screen::initDraw() {
        m_color |= 0xff000000; // set alpha to fully opaque
        m_lineWidth = 1.f;

    }

    bool Screen::initDrawOp(float x, float y, bool objectTransform) {
        if (!m_canvas->isReady()) return false;
        bool restore = false;
        if (m_screenTransform) {
            m_canvas->push();
            restore = true;
            m_canvas->translate(m_transX, -m_transY);
            m_canvas->scale(m_scaleX, m_scaleY);
            m_canvas->rotate(-m_degrees * deg2rad);
        }
        if (objectTransform && (m_objectDegrees != 0.f || m_objectScaleX != 1.f || m_objectScaleY != 1.f)) {
            if (!restore) {
                m_canvas->push();
                restore = true;
                m_canvas->translate(x, y);
            }
            if (m_objectDegrees != 0.f)
                m_canvas->rotate(-m_objectDegrees * deg2rad);
            if (m_objectScaleX != 1.f || m_objectScaleY != 1.f)
                m_canvas->scale(m_objectScaleX, m_objectScaleY);
        }
        return restore;
    }

    void Screen::closeDrawOp() {
        if (!m_canvas->isReady()) return;
        m_canvas->pop();
    }

    void Screen::setColor(uint32_t color) {
        m_color = (m_color & 0xFF000000) | (color & 0xFFFFFF);
        if (!m_canvas->isReady()) return;
        m_canvas->setFillColor(ARGBtoABGR(m_color));
        m_canvas->setStrokeColor(ARGBtoABGR(m_color));
    }

    void Screen::setAlpha(int value) {
        m_color = (m_color & 0xFFFFFF) | ((value & 255) << 24);
        if (!m_canvas->isReady()) return;
        m_canvas->setFillColor(ARGBtoABGR(m_color));
        m_canvas->setStrokeColor(ARGBtoABGR(m_color));
    }

    void Screen::clear(uint32_t color) {
        if (!m_canvas->isReady()) return;
        m_canvas->clearWithColor(ARGBtoABGR(color));
    }

    void Screen::setDrawAnchor(float x, float y) {
        m_anchorX = x;
        m_anchorY = y;
        // remap microstudio anchors (-1 -> 0 -> 1) to 0 -> 0.5 -> 1
        m_canvas->setDrawAnchors(.5 + (x * .5), .5 + (y * .5));
    }

    void Screen::setDrawRotation(float deg) {
        m_objectDegrees = deg;
    }

    void Screen::drawSprite(std::string_view name, float x, float y, float w, float h) {
        int frameNum = -1;
        size_t pFrame = name.rfind('.');
        if (pFrame != std::string::npos) {
            auto frameStr = name.substr(pFrame + 1);
            frameNum = atoi(frameStr.data());
            name = name.substr(0, pFrame); // fix up name for getSpritePath...
        }
        
        std::string_view path = getRuntime()->getSpritePath(name);
        if (path.empty()) return;

        res::Image::AtlasRect r;
        auto texture = m_atlas->toTexture();
        if (!m_atlas->findAtlasRect(path, r)) {
            // if somehow the atlas doesn't have that sprite we'll have to do a lame slow swap to a separate image
            auto image = res::ResourceManager::get()->getCached<res::Image>(path);
            if (image) {
                LOG_MSGF("Sprite at \"%s\" not in main atlas! Need to draw from isolated texture!\n", path.data());
                texture = image->toTexture();
                r.x = r.y = 0;
                r.width = image->getWidth();
                r.height = image->getHeight();
                r.fps = image->getFPS();
                r.nframes = image->getFrameCount();
            } else {    
                LOG_MSGF("Sprite at \"%s\" not found in atlas or loaded resources!\n", path.data());
            }
        }

        // pick a frame if we have to draw an animated sprite
        if (r.nframes > 1) {
            r.height /= r.nframes;
            if (frameNum >= 0)
                r.y += r.height * (frameNum % r.nframes);
            else {
                // slightly more expensive process when we have to check if the user defined a current frame
                auto img = getRuntime()->getSpriteImage(path);
                r.y += r.height * img->getAnimCurrentFrame();
            }
        }

        // finally, draw
        if (initDrawOp(x, -y)) {
            m_canvas->drawQuad(texture.operator->(), r.x, r.y, r.width, r.height,
            0.f, 0.f, w, h);
            closeDrawOp();
        } else {
            m_canvas->drawQuad(texture.operator->(), r.x, r.y, r.width, r.height,
            x, -y, w, h);
        }
    }

    void Screen::fillRect(float x, float y, float w, float h) {
        m_canvas->fillRect(x, y, w, h);
    }

    void Screen::drawText(char const* text, float x, float y, float sz) {
        const float ftBias = 1.2f; // artificially match ms' font render size (honestly just eyeball work)
        m_canvas->drawText(text, x, y, static_cast<int>(sz * ftBias), m_ratio);
    }

    void Screen::drawLine(float x0, float y0, float x1, float y1) {
        // anchor is not taken into account for lines
        m_canvas->drawLine(x0, y0, x1, y1);
    }

    void Screen::setLineWidth(float w) {
        m_canvas->setLineWidth(w);
    }

    uint32_t Screen::stringToColor(char const* str) {
        int r, g, b;
        if (sscanf(str, "rgb(%d ,%d ,%d)", &r, &g, &b) == 3) {
            return (((r & 255) << 16) | ((g & 255) << 8) | (b & 255)) | 0xFF000000;
        }
        int len = 0;
        if (sscanf(str, "#%X%n", &r, &len)) {
            switch (len) {
                case 4: // #RGB notation
                return r << 12 | r << 8 | ((r & 7) << 4);
                case 7: // #RRGGBB notation
                return (r & 0xFFFFFF) | 0xFF000000;
                case 9: // #RRGGBBAA notation
                return ((r & 0xFFFFFF00) >> 8) | ((r & 255) << 24);
            }
        }
        return 0u;
    }

    uint32_t Screen::decimalToColor(int dec) {
        int r = (((dec / 100) % 10) / 9) * 255;
        int g = (((dec / 10) % 10) / 9) * 255;
        int b = ((dec % 10) / 9) * 255;
        return (r << 16) + (g << 8) + b;
    }

    uint32_t Screen::ARGBtoABGR(uint32_t color) {
        return  ((color & 0xff0000) >> 16)| // R
                ((color & 0xff) << 16)|     // B
                color & 0xff00ff00;         // G + A
    }

}
}
