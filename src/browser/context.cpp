#include "context.hpp"
#include "core/util.hpp"

namespace ms {
namespace browser {

    void Context::init() {
        m_scriptHost.createEngine();
    }

    void Context::free() {
        m_cleanupQueue.flushNotifications();
        m_scriptHost.destroyEngine();
        if (m_renderer.isReady())
            m_renderer.free();
    }

    void Context::addRepaintListener(EventQueue::EventFuncType slot) {
        m_repaintQueue.queueNotification(slot);
    }

    void Context::addRepaintListener(EventQueue::EventFuncType slot, EventQueue::EventFuncType cleanup) {
        m_repaintQueue.queueNotification(slot);
        m_cleanupQueue.queueNotification(cleanup);
    }

    void Context::processRepaintNotifications() {
        m_repaintQueue.flushNotifications();
        m_cleanupQueue.flushNotifications();
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
