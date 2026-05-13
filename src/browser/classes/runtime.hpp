#pragma once

#include "core/events.hpp"
#include "core/robin_hood.hpp"
#include "platform/input.hpp"

namespace ms {
namespace browser {

    class Screen;

    class Runtime {
    public:
    enum Orientation {
        Landscape,
            Portrait
        } m_orientation;
        enum AspectRatio {
            AR_Unknown,
            AR_4x3,
            AR_16x9,
            AR_2x1,
            AR_1x1,
            AR_MinValues,
            AR_M4x3,
            AR_M16x9,
            AR_M2x1,
            AR_M1x1,
        } m_aspect;
    protected:
        Screen* m_screen;

        bool m_started;

        robin_hood::unordered_map<
            std::string, std::string
        > m_spriteNameMap;

        void mapSpriteNames();

        platform::InputManager* m_input;
        bool m_isTouching;
        int m_inputPointerHandler;
    
    public:
        Runtime();
        ~Runtime();

        /** Get sprite path from name, for drawing */
        std::string getSpritePath(std::string const& path) const;

        /** Must only be called by script constructor */
        void setScreen(Screen* screen) { m_screen = screen; }

        Screen* getScreen() const { return m_screen; }

        /** Get aspect ratio scalar from enum value */
        static inline float getRatioFor(AspectRatio enumval);

        /** Get device orientation */
        Orientation getOrientation() const { return m_orientation; }

        /** Get device aspect ratio */
        AspectRatio getAspectRatio() const { return m_aspect; }

        void startReady();

        void checkStartReady();

        void start();

        void timer();

        //void updateCall();

        void updateControls();

        /** Exit runtime */
        void exit();

        /* INPUT */

        bool isTouching() const { return m_isTouching; }

    public:
        Event<> startVM;
        Event<> timerStep;
        Event<> updatedControls;
    };

    float Runtime::getRatioFor(AspectRatio enumval)  {
        switch (enumval) {
            case AR_16x9: case AR_M16x9: return 16.f / 9.f;
            case AR_4x3: case AR_M4x3: return 4.f / 3.f;
            case AR_2x1: case AR_M2x1: return 2.f;
            case AR_1x1: case AR_M1x1: return 1.f;
            default: return 1.f;
        }
    }

}
}
