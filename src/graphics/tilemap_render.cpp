#include "tilemap_render.hpp"

namespace ms {
namespace gfx {

    /**
     * Populate the render data from the given tilemap.
     */
    void TilemapRenderData::createFromTilemap(res::TileMap const& tilemap) {
        clearLayers();
        
    }

    /**
     * Build data from each layer where such layer is defined by a texture that may be animated.
     * All inanimate textures should be merged into a single layer.
     */
    void TilemapRenderData::buildRenderData(CanvasRC2D* canvas) {
        
    }

    /**
     * Render the data that was built ahead of time.
     */
    void TilemapRenderData::render(CanvasRC2D* canvas) {
        
    }

}
}
