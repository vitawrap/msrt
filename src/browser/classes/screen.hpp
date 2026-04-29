#pragma once

#include <stdint.h>
#include "graphics/canvas2d_rc.hpp"

namespace ms {
namespace browser {

    /**
     * @brief Microstudio screen class (default 2D graphics interface)
     */
    class Screen {
        gfx::CanvasRC2D* m_canvas;

        /** Virtual width and height */
        int m_width, m_height;
    
    public:
        /** Grab context */
        void initContext();

        /** Clear with color */
        void clear(uint32_t color);

        /** Set color state */
        void setColor(uint32_t color);

        /** Translate a color notation string to an actual color */
        static uint32_t stringToColor(char const* str);

        /** Short decimal notation (000-999) to conventional color */
        static uint32_t decimalToColor(int dec);
        
    };

}
}
