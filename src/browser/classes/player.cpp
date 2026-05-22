#include "player.hpp"

#include "core/app.hpp"
#include "screen.hpp"
#include "resources/ms_script.hpp"

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

    void Player::loadSources() {
        res::Project* pro = Application::get()->getProject();
        if (pro) {
            auto const& scripts = pro->getScriptMap();
            for (auto const& entry : scripts) {
                sourceFileAdded.invoke(entry.first, entry.second->getText());
            }
        }
    }

    void Player::start() {
        // at this point Runtime has been allocated by the script realm
        loadSources();

        auto* wm = Application::get()->getWindowManager();
        m_resizeListener = wm->resized.connect([this](int windowId, int w, int h){
            resize();
        }).id;
        resize();
        if (m_runtime)
            m_runtime->start();
        resize(); // resize a second time so it catches up on aspect and orientation...
    }

    void Player::resize() {
        if (Screen* screen = m_runtime->getScreen())
            screen->resize();
        needRedraw.invoke(); // call into script
    }

}
}