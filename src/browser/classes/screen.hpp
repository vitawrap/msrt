#pragma once

#include <stdint.h>
#include "runtime.hpp"
#include "graphics/canvas2d_rc.hpp"
#include "resources/resource_manager.hpp"
#include "resources/ms_image.hpp"

namespace ms {
namespace browser {

    /**
     * @brief Microstudio screen class (default 2D graphics interface)
     */
    class Screen {
        /** Runtime reference, allocated by script */
        Runtime* m_runtime;
        gfx::CanvasRC2D* m_canvas;
        
        bool m_screenTransform;

        /** Virtual width and height */
        int m_width, m_height;
        /** Color + alpha */
        uint32_t m_color;
        /** Stroke line width */
        float m_lineWidth;
        /** Transform values */
        float m_transX, m_transY, m_scaleX, m_scaleY, m_degrees;
        /** Object transform values */
        float m_objectScaleX, m_objectScaleY, m_objectDegrees;
        /** Draw anchor */
        float m_anchorX, m_anchorY;

        /** Main atlas for drawing */
        res::ResourceHandle<res::Image> m_atlas;
    
    public:
        Screen();

        /** Must only be called by script constructor */
        void setRuntime(Runtime* rt) { m_runtime = rt; }

        /** Grab context */
        void initContext();

        /** Clear with color */
        void clear(uint32_t color);

        /** Set color state */
        void setColor(uint32_t color);

        /** Get last screen color */
        uint32_t getColor() const { return m_color; }

        /** Set alpha state */
        void setAlpha(int value);

        /** Draw a sprite frame on screen */
        void drawSprite(std::string_view name, float x, float y, float w, float h);

        /** Draw a filled rectangle */
        void fillRect(float x, float y, float w, float h);

        /** Resize canvas and virtual viewport */
        void resize();

        /** Set draw anchor */
        void setDrawAnchor(float x, float y);

        /** ? */
        void initDraw();

        /** Returns true if context has to be restored (closeDrawOp) */
        bool initDrawOp(float x, float y, bool objectTransform = true);

        /** Restore context */
        void closeDrawOp();

        /** Originally serves as to register all pointing events */
        void startControl();

        /** Translate a color notation string to an actual color (ARGB) */
        static uint32_t stringToColor(char const* str);

        /** Short decimal notation (000-999) to conventional color (ARGB) */
        static uint32_t decimalToColor(int dec);

        /** Convert web hex color to raylib color */
        static uint32_t ARGBtoABGR(uint32_t hex);
        
        /** Get the runtime controlling this screen */
        Runtime* getRuntime() const { return m_runtime; }
    };

}
}
