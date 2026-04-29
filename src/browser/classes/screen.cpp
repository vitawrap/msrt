#include "screen.hpp"
#include "core/app.hpp"
#include <algorithm>

namespace ms {
namespace browser {

    void Screen::initContext() {
        auto* browser = Application::get()->getBrowserContext();
        m_canvas = browser->getCanvas();
        m_canvas->push();
        m_canvas->translate(m_canvas->getWidth() >> 1, m_canvas->getHeight() >> 1);
        float ratio = std::min(m_canvas->getWidth() / 200, m_canvas->getHeight() / 200);
        m_canvas->scale(ratio, ratio);
        m_width = m_canvas->getWidth() / ratio;
        m_height = m_canvas->getHeight() / ratio;
    }

    void Screen::setColor(uint32_t color) {
        /** TODO: */
    }

    void Screen::clear(uint32_t color) {
        /** TODO: */
    }

    uint32_t Screen::stringToColor(char const* str) {
        int r, g, b;
        if (sscanf(str, "rgb(%d ,%d ,%d)", &r, &g, &b) == 3) {
            return (((r & 255) << 16) | ((g & 255) << 8) | (b & 255)) | 0xFF000000;
        }
        int len = 0;
        if (sscanf(str, "#%X%n", &r, &len)) {
            if (len == 4) { // #RGB notation
                return r << 12 | r << 8 | ((r & 7) << 4);
            } else if (len == 7) { // #RRGGBB notation
                return (r & 0xFFFFFF) | 0xFF000000;
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

}
}
