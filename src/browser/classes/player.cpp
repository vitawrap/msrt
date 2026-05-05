#include "player.hpp"

#include "core/app.hpp"
#include "screen.hpp"

namespace ms {
namespace browser {

    Player::Player() :
        m_runtime(nullptr),
        m_resizeListener(0)
    {
    }

    Player::~Player() {
        if (m_resizeListener)
            Application::get()->getWindowManager()->resized
                .disconnect(m_resizeListener);
    }

    void Player::start() {
        // at this point Runtime has been allocated by the script realm

        auto* wm = Application::get()->getWindowManager();
        m_resizeListener = wm->resized.connect([this](int windowId, int w, int h){
            resize();
        }).id;
        resize();
        if (Runtime* rt = m_runtime)
            m_runtime->start();
    }

    void Player::resize() {
        if (Screen* screen = m_runtime->getScreen())
            screen->resize();
        needRedraw.invoke(); // call into script
    }

}
}