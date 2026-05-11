#include "screen.hpp"
#include "core/app.hpp"
#include "resources/resource_manager.hpp"
#include <algorithm>

namespace ms {
namespace browser {

    constexpr float deg2rad = 57.2957795131f;

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
        m_lineWidth(1.f),
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
        m_canvas->translate(m_canvas->getWidth() >> 1, m_canvas->getHeight() >> 1);
        float ratio = std::min(m_canvas->getWidth() / 200, m_canvas->getHeight() / 200);
        m_canvas->scale(ratio, ratio);
        m_width = m_canvas->getWidth() / ratio;
        m_height = m_canvas->getHeight() / ratio;
    }

    void Screen::resize() {
        auto arType = m_runtime->getAspectRatio();
        float arValue = Runtime::getRatioFor(arType);
        bool min = arType > Runtime::AR_MinValues;
        if (arType != Runtime::AR_Unknown) {

        }
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
        m_color = color;
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

    void Screen::drawSprite(std::string const& name, float x, float y, float w, float h) {
        
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
