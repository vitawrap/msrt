#pragma once

#include "platform.hpp"
#include "core/events.hpp"

namespace ms {
namespace platform {

    class IInputManager {
    public:
        struct KeyInfo {
            int keyCode;
            bool pressed;

            KeyInfo(int kc, bool p) : keyCode(kc), pressed(p) {}
        };

        enum PointerDevice {
            PD_MOUSE,
            PD_TOUCH
        };

        struct PointerInfo {
            PointerDevice device;
            int button;         // always 0 for touch
            short x, y;
            bool pressed;

            PointerInfo(PointerDevice pt, int btn, short x, short y, bool prs):
                device(pt), button(btn), x(x), y(y), pressed(prs) {}
        };

        struct PointerMoveInfo {
            PointerDevice device;
            int x, y;
        };

        /** Must be available from implementers */
        IInputManager() {}

        virtual void pollEvents() = 0;

        /** Event for key presses and releases */
        Event<KeyInfo> key;

        /** Pointer events (mouse, touch, ...) */
        Event<PointerInfo> pointer;
        Event<PointerMoveInfo> pointerMove;
    };

}
}

#if __has_include("abstracted/input.hpp")
#include "abstracted/input.hpp"
#elif defined(PLATFORM_UNIX)
#include "unix/input.hpp"
#elif defined(PLATFORM_NT)
#include "nt/input.hpp"
#elif defined(PLATFORM_DARWIN)
#include "darwin/input.hpp"
#elif defined(PLATFORM_BROWSER)
#include "browser/input.hpp"
#else
#error "TODO: Windowing for this platform"
#endif
