#include "runtime.hpp"
#include "core/app.hpp"
#include "io/log.hpp"

namespace ms {
namespace browser {

    Runtime::Runtime():
        m_screen(nullptr),
        m_started(false)
    {}

    void Runtime::startReady() {
        m_started = true;
        startVM.invoke(); // call into script
    }

    void Runtime::checkStartReady() {
        if (!m_started)
            startReady();
    }

    void Runtime::start() {
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

    }

    void Runtime::exit() {
        Application::get()->quit();
    }

}
}
