#include "context.hpp"
#include "core/util.hpp"

namespace ms {
namespace browser {

    void Context::init() {
        m_scriptHost.createEngine();
    }

    void Context::free() {
        m_scriptHost.destroyEngine();
    }

    void Context::addRepaintListener(EventQueue::EventFuncType slot) {
        m_repaintQueue.queueNotification(slot);
    }

    void Context::processRepaintNotifications() {
        m_repaintQueue.flushNotifications();
    }

    void Context::setDOMRoot(Node* gcPtr) {
        DEBUG_ASSERT((!m_domRoot) && "DOM root was not null before setDOMRoot!");
        m_domRoot = gcPtr;
    }

    void Context::repaint(platform::IWindowManager* wm) {

    }

}
}
