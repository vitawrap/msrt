#include "context.hpp"
#include "core/util.hpp"

namespace ms {
namespace browser {

    void Context::init() {
        m_scriptHost.createEngine();
    }

    void Context::free() {
        m_scriptHost.destroyEngine();
        if (m_renderer.isReady())
            m_renderer.free();
    }

    void Context::addRepaintListener(EventQueue::EventFuncType slot) {
        m_repaintQueue.queueNotification(slot);
    }

    void Context::processRepaintNotifications() {
        m_repaintQueue.flushNotifications();
    }

    void Context::setupRepaint(platform::IWindowManager* wm) {
        if (!m_renderer.isReady())
            m_renderer.init(wm);
        m_renderer.beginFrame();
    }

    void Context::repaint() {
        m_renderer.submitFrame();
    }

}
}
