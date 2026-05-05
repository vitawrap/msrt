#pragma once

#include "runtime.hpp"

namespace ms {
namespace browser {

    /**
     * "Player" in the sense of microstudio is the frontend of the Runtime.
     */
    class Player {
        /** Player owns the runtime object */
        Runtime* m_runtime;
        int m_resizeListener;

    public:
        /** Must only be called by script constructor */
        void setRuntime(Runtime* rt) { m_runtime = rt; }

        Player();
        ~Player();
        
        void start();

        void resize();

        Runtime* getRuntime() const { return m_runtime; }

        Event<> needRedraw;
    };

}
}
