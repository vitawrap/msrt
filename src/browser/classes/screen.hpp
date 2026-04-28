#pragma once

#include "graphics/canvas2d_rc.hpp"

namespace ms {
namespace browser {

    /**
     * @brief Microstudio screen class (default 2D graphics interface)
     */
    class Screen {
        gfx::CanvasRC2D* m_canvas;
    
    public:
        /** Grab context */
        void initContext();
        
    };

}
}
