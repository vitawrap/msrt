#pragma once

#include "core/events.hpp"
#include "platform/windowing.hpp"
#include "script_host.hpp"
#include "graphics/canvas2d_rc.hpp"

namespace ms {
namespace browser {

    class Context {
        EventQueue m_repaintQueue;
        ScriptHost m_scriptHost;

        /** Implements and consolidates features similar to a browser canvas */
        gfx::CanvasRC2D m_renderer;

    public:
        Context() :
            m_scriptHost(this)
        {}

        /** Get the underlying javascript engine */
        ScriptHost* getScriptHost() { return &m_scriptHost; }

        /** Initialize the browser context */
        void init();

        /** Destroy the browser context */
        void free();

        /** add listener to be invoked BEFORE a repaint */
        void addRepaintListener(EventQueue::EventFuncType slot);

        /** flush notifications in repaint queue */
        void processRepaintNotifications();

        /** set up repaint for this frame */
        void setupRepaint(platform::IWindowManager* wm);

        /** Request to repaint current state of browser */
        void repaint();

        /** Get reference to canvas used in this context */
        gfx::CanvasRC2D* getCanvas() { return &m_renderer; }
    };

}
}
