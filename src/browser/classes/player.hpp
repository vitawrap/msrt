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

        /** The current player instance */
        static Player* s_instance;

    public:
        /** Must only be called by script constructor */
        void setRuntime(Runtime* rt) { m_runtime = rt; }

        Player();
        ~Player();

        void loadSources();
        
        void start();

        void resize();

        Runtime* getRuntime() const { return m_runtime; }

        /** Player* is null when JS layer is not initialized! */
        static Player* current() { return s_instance; }

        Event<std::string, std::string> sourceFileAdded;
        Event<> needRedraw;
    };

}
}
