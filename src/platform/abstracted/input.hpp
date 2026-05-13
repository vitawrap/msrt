#pragma once

#include "../input.hpp"

namespace ms {
namespace platform {

    class InputManager : public virtual IInputManager {
        /** Keep track of keys pressed */
        std::vector<KeyInfo> m_keysPressed;

        /** Keep track of mouse/touch currently pressed */
        std::vector<PointerInfo> m_pointersPressed;

    public:
        /** Poll all input events */
        void pollEvents() override;
        
        
    };

}
}
