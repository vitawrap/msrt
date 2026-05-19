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
        m_isTouching(false),
        m_inputPointerHandler(0)
    {}

    void Runtime::startReady() {
        // hook into input manager
        m_input = Application::get()->getInputManager();
        DEBUG_ASSERT(m_input);
        m_inputPointerHandler = m_input->pointer.connect([this](auto pi) {
            m_isTouching = Application::get()->getInputManager()->isPointerPressed();
        }).id;

        m_started = true;
        startVM.invoke(); // call into script
    }

    Runtime::~Runtime() {
        if (m_input) {
            m_input->pointer.disconnect(m_inputPointerHandler);
        }
    }

    void Runtime::checkStartReady() {
        if (!m_started)
            startReady();
    }

    std::string_view Runtime::getSpritePath(const std::string& name) const {
        auto itr = m_spriteNameMap.find(name);
        if (itr != m_spriteNameMap.cend())
            return itr->second;
        return "";
    }

    void Runtime::mapSpriteNames() {
        // remap sprite resource names to sprite paths for use in screen commands
        auto* project = Application::get()->getProject();
        auto const& sprMap = project->getSpriteMap();
        for (const auto& [pathStr, _] : sprMap) {
            char stemBuffer[256] = {0}; // frankly easier than std for such tasks
            sscanf(pathStr.c_str(), "sprites/%255[^.]s", stemBuffer);
            m_spriteNameMap.emplace(stemBuffer, pathStr);
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
    }

    void Runtime::exit() {
        Application::get()->quit();
    }

}
}
