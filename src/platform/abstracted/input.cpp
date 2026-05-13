
#include "platform/input.hpp"
#include "core/util.hpp"
#include <raylib.h>
#include <math.h>

namespace ms {
namespace platform {

    void InputManager::pollEvents() {
        // release keys
        for (auto itr = m_keysPressed.begin(); itr != m_keysPressed.end();) {
            if (IsKeyReleased(itr->keyCode)) {
                KeyInfo& ki = *itr;
                ki.pressed = false;
                key.invokeDeferred(ki);
                itr = m_keysPressed.erase(itr);
            }
            else
                ++itr;
        }

        // press keys
        int keyCode;
        while ((keyCode = GetKeyPressed()) != 0) {
            m_keysPressed.emplace_back(keyCode, true);
            key.invokeDeferred(m_keysPressed.back());
        }

        // release mouse buttons
        Vector2 ptPos = GetMousePosition();
        for (auto itr = m_pointersPressed.begin(); itr != m_pointersPressed.end();) {
            if (itr->device == PD_MOUSE && IsMouseButtonReleased(itr->button)) {
                PointerInfo& pi = *itr;
                pi.pressed = false;
                pi.x = static_cast<short>(roundf(ptPos.x));
                pi.y = static_cast<short>(roundf(ptPos.y));
                pointer.invokeDeferred(pi);
                itr = m_pointersPressed.erase(itr);
            }
            else
                ++itr;
        }

        // catch mouse moving
        Vector2 ptDt = GetMouseDelta();
        if (ptDt.x != 0.0 && ptDt.y != 0.0) {
            PointerMoveInfo pmi;
            pmi.device = PD_MOUSE;
            pmi.x = ptPos.x;
            pmi.y = ptPos.y;
            pointerMove.invokeDeferred(pmi);
        }

        // press mouse buttons
        for (int i = MouseButton::MOUSE_BUTTON_LEFT; i < MouseButton::MOUSE_BUTTON_BACK; ++i) {
            if (IsMouseButtonPressed(i)) {
                m_pointersPressed.emplace_back(PD_MOUSE, i, ptPos.x, ptPos.y, true);
                pointer.invokeDeferred(m_pointersPressed.back());
            }
        }
    }

}
}
