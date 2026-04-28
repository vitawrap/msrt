#include "screen.hpp"
#include "core/app.hpp"

namespace ms {
namespace browser {

    void Screen::initContext() {
        auto* browser = Application::get()->getBrowserContext();
        m_canvas = browser->getCanvas();
    }

}
}
