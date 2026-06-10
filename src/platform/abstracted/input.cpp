
#include "platform/input.hpp"
#include <ms/core/util.hpp>
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <math.h>

namespace ms {
namespace platform {

    /**
     * @brief Get a name for the specified keycode and optionally its printable version 
     * @param keyCode Raylib/GLFW KEY_*
     * @param print Printable symbol of key
     * @param name Name associated with key
     * @return whether or not the keycode was printable
     */
    static bool getKeyNameChar(int keyCode, char const*& name, char const*& print) {
        /* GLFW key names are *almost* exactly what microStudio uses as key names */
        char const* def = glfwGetKeyName(keyCode, glfwGetKeyScancode(keyCode));
        if (!def) {
            switch (keyCode) {
                case KEY_SPACE: name = "space", print = " "; return true;
                case KEY_LEFT_SHIFT: case KEY_RIGHT_SHIFT: name = "shift"; return false;
                case KEY_LEFT_CONTROL: case KEY_RIGHT_CONTROL: name = "control"; return false;
                case KEY_INSERT: name = "insert"; return false;
                case KEY_DELETE: name = "delete"; return false;
                case KEY_ESCAPE: name = "escape"; return false;
                case KEY_LEFT_ALT: name = "alt"; return false;
                case KEY_RIGHT_ALT: name = "altgraph"; return false;
                case KEY_TAB: name = "tab"; return false;
                case KEY_ENTER: name = "enter"; return false;
                case KEY_BACKSPACE: name = "backspace"; return false;
                case KEY_HOME: name = "home"; return false;
                case KEY_PAGE_UP: name = "pageup"; return false;
                case KEY_PAGE_DOWN: name = "pagedown"; return false;
                case KEY_END: name = "end"; return false;
                case KEY_LEFT: name = "arrowleft"; return false;
                case KEY_RIGHT: name = "arrowright"; return false;
                case KEY_UP: name = "arrowup"; return false;
                case KEY_DOWN: name = "arrowdown"; return false;
                case KEY_CAPS_LOCK: name = "capslock"; return false;
            }
            return false;
        }
        name = def;
        print = def;
        return true;
    }

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
            char const *name = nullptr, *print = nullptr;
            getKeyNameChar(keyCode, name, print);
            m_keysPressed.emplace_back(keyCode, name, print, true);
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
