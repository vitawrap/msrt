#include "runtime.hpp"
#include "core/app.hpp"
#include "io/log.hpp"

#include <stdio.h>

namespace ms {
namespace browser {

    Runtime::Runtime():
        m_screen(nullptr),
        m_started(false),
        m_input(nullptr),
        m_inputPointerHandler(0),
        m_inputKeyHandler(0)
    {
        m_touch.isPressed = false;
        m_touch.isReleased = false;
        m_touch.isTouching = false;
        m_touch.x = m_touch.y = 0;
    }

    static Runtime::AspectRatio strToAspectRatio(std::string_view aspect) {
        if (aspect == "free") return Runtime::AR_Unknown;
        if (aspect == "4x3") return Runtime::AR_4x3;
        if (aspect == "16x9") return Runtime::AR_16x9;
        if (aspect == "2x1") return Runtime::AR_2x1;
        if (aspect == "1x1") return Runtime::AR_1x1;
        if (aspect == ">4x3") return Runtime::AR_M4x3;
        if (aspect == ">16x9") return Runtime::AR_M16x9;
        if (aspect == ">2x1") return Runtime::AR_M2x1;
        if (aspect == ">1x1") return Runtime::AR_M1x1;
        return Runtime::AR_Unknown;
    }

    void Runtime::startReady() {
        // get view info from project
        auto& settings = Application::get()->getProject()->getSettings();
        m_orientation = settings.orientation == "landscape"? Landscape : Portrait;
        m_aspect = strToAspectRatio(settings.aspect);

        // hook into input manager
        m_input = Application::get()->getInputManager();
        DEBUG_ASSERT(m_input);
        m_inputPointerHandler = m_input->pointer.connect([this](auto pi) {
            m_touch.isTouching = Application::get()->getInputManager()->isPointerPressed();
            if (m_touch.isTouching) {
                m_touch.x = pi.x;
                m_touch.y = pi.y;
                m_touch.isPressed = true;
            } else {
                m_touch.isReleased = true;
            }
        }).id;
        m_inputKeyHandler = m_input->key.connect([this](auto ki) {
            if (ki.pressed) {
                LOG_MSGF("Key pressed: %s\n", ki.name);
                if (ki.name) m_keys.current.emplace(ki.name, KS_PRESS);
                if (ki.print) m_keys.current.emplace(ki.print, KS_PRESS);
            }
            else {
                if (ki.name) m_keys.current.emplace(ki.name, KS_RELEASE);
                if (ki.print) m_keys.current.emplace(ki.print, KS_RELEASE);
            }
        }).id;

        m_started = true;
        startVM.invoke(); // call into script
    }

    Runtime::~Runtime() {
        if (m_input) {
            m_input->pointer.disconnect(m_inputPointerHandler);
            m_input->key.disconnect(m_inputKeyHandler);
        }
    }

    void Runtime::checkStartReady() {
        if (!m_started)
            startReady();
    }

    std::string_view Runtime::getSpritePath(std::string_view name) const {
        auto itr = m_spriteNameMap.find(name);
        if (itr != m_spriteNameMap.cend())
            return itr->second;
        return "";
    }

    res::ResourceHandle<res::Image> Runtime::getSpriteImage(std::string_view path) const {
        auto itr = m_spriteImageMap.find(path);
        if (itr != m_spriteImageMap.cend())
            return itr->second;
        return res::ResourceHandle<res::Image>{nullptr};
    }

    void Runtime::mapSpriteNames() {
        // remap sprite resource names to sprite paths for use in screen commands
        auto* project = Application::get()->getProject();
        auto const& sprMap = project->getSpriteMap();
        for (const auto& [pathStr, res] : sprMap) {
            char stemBuffer[256] = {0}; // frankly easier than std for such tasks
            sscanf(pathStr.c_str(), "sprites/%255[^.]s", stemBuffer);
            m_spriteNameMap.emplace(stemBuffer, pathStr);
            m_spriteImageMap.emplace(pathStr, res);

            // give script realm initial sprite info
            spriteMapped.invoke(pathStr, res.operator->());
        }
    }

    void Runtime::start() {
        mapSpriteNames();
        checkStartReady();
    }

    void Runtime::timer() {
        Context* browser = Application::get()->getBrowserContext();
        browser->addRepaintListener([this](){ // same as requestAnimationFrame
            timer();
        });
        timerStep.invoke(); // call into script
    }

    void Runtime::updateControls() {
        // update from inputmanager events
        updatedControls.invoke(); // call into script
        
        m_touch.isPressedFrame = m_touch.isPressed;
        m_touch.isReleasedFrame = m_touch.isReleased;
        m_touch.isPressed = false;
        m_touch.isReleased = false;

        m_keys.frame = m_keys.current;
        for (auto& [k, ks] : m_keys.frame) {
            if (ks == KS_RELEASE) {
                m_keys.current.erase(k);
            } else if (ks == KS_PRESS) {
                m_keys.current[k] = KS_DOWN;
            }
        }
    }

    void Runtime::exit() {
        Application::get()->quit();
    }

}
}
