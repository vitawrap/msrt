#pragma once

#include "platform/windowing.hpp"

namespace ms {
namespace gfx {

    /**
     * @brief Base class over canvas rendering contexts.
     * TODO: Multi-threaded rendering jobs/cmd queue
     */
    class CanvasRC {
        
    public:
        virtual ~CanvasRC() {}
        virtual bool isReady() const { return false; }
        virtual void init(platform::IWindowManager* wm, int wid = 0) = 0;
        virtual void free() = 0;

        /** Save canvas drawing state */
        virtual void push() = 0;
        /** Restore canvas drawing state */
        virtual bool pop() = 0;

        virtual int getWidth() const = 0;
        virtual int getHeight() const = 0;

        virtual platform::IWindowManager* getWindowManager() const = 0;
        virtual int getWindowID() const = 0;

        virtual void beginFrame() = 0;
        virtual void submitFrame() = 0;
    };

}
}