#pragma once

#include "core/events.hpp"
#include "platform/windowing.hpp"
#include "script_host.hpp"
#include "node.hpp"

namespace ms {
namespace browser {

    class Context {
        EventQueue m_repaintQueue;
        ScriptHost m_scriptHost;
        Node m_domRoot;

    public:
        Context()
            : m_scriptHost(this)
        {}

        /** Get the underlying javascript engine */
        ScriptHost* getScriptHost() { return &m_scriptHost; }

        /** Initialize the browser context */
        void init();

        /** Destroy the browser context */
        void free();

        /** Get simulated DOM root (no document system) */
        Node const* getDOM() const { return &m_domRoot; }
        Node* getDOM() { return &m_domRoot; }

        /** add listener to be invoked BEFORE a repaint */
        void addRepaintListener(EventQueue::EventFuncType slot);

        /** flush notifications in repaint queue */
        void processRepaintNotifications();

        /** Request to repaint current state of browser */
        void repaint(platform::IWindowManager* wm);
    };

}
}
