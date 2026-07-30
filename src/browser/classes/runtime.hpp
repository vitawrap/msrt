#pragma once

#include <stdint.h>

#include "core/events.hpp"
#include <ms/core/view_map.hpp>
#include <ms/core/static_map.hpp>
#include "platform/input.hpp"
#include "resources/ms_image.hpp"
#include "resources/ms_tilemap.hpp"

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

        ms::unordered_map<std::string> m_spriteNameMap;
        ms::unordered_map<res::ResourceHandle<res::Image>> m_spriteImageMap;

        ms::unordered_map<res::ResourceHandle<res::TileMap>> m_tilemapMap;

        void mapSpriteNames();
        void mapTilemapNames();

        platform::InputManager* m_input;
        int m_inputPointerHandler;
        int m_inputKeyHandler;

        struct {
            float x, y;
            bool isTouching  : 1;
            bool isPressed   : 1;
            bool isReleased  : 1;
            bool isPressedFrame : 1;
            bool isReleasedFrame : 1;
        } m_touch;

        enum KeyState {
            KS_PRESS,
            KS_DOWN,
            KS_RELEASE,
        };

        struct {
            ms::unordered_map_static<KeyState> current;
            ms::unordered_map_static<KeyState> frame;
        } m_keys;
    
    public:
        Runtime();
        ~Runtime();

        /** Get sprite path from name, for drawing */
        std::string_view getSpritePath(std::string_view path) const;

        /** Get reference to a loaded tilemap */
        res::ResourceHandle<res::TileMap> getTilemap(std::string_view path) const;

        /** Get sprite image from path, for drawing */
        res::ResourceHandle<res::Image> getSpriteImage(std::string_view path) const;

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

        bool isTouching() const { return m_touch.isTouching; }
        bool isTouchPressed() const { return m_touch.isPressedFrame; }
        bool isTouchReleased() const { return m_touch.isReleasedFrame; }
        float getTouchX() const { return m_touch.x; }
        float getTouchY() const { return m_touch.y; }

        bool isKeyDown(char const* name) const {
            return m_keys.frame.contains(name) && m_keys.frame.at(name) != KS_RELEASE;
        }

        bool isKeyUp(char const* name) const {
            return name && (!m_keys.frame.contains(name) || isKeyReleased(name));
        }

        bool isKeyReleased(char const* name) const {
            return m_keys.frame.contains(name) && m_keys.frame.at(name) == KS_RELEASE;
        }

        bool isKeyPressed(char const* name) const {
            return m_keys.frame.contains(name) && m_keys.frame.at(name) == KS_PRESS;
        }      

        size_t keyCount() const { return m_keys.frame.size(); }
        auto keysBegin() const { return m_keys.frame.cbegin(); }
        auto keysBegin() { return m_keys.frame.begin(); }
        auto keysEnd() const { return m_keys.frame.cend(); }
        auto keysEnd() { return m_keys.frame.end(); }

    public:
        Event<> startVM;
        Event<> timerStep;
        Event<> updatedControls;
        Event<std::string_view, res::Image const*> spriteMapped;
        Event<std::string_view, res::TileMap const*> tilemapMapped;
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
