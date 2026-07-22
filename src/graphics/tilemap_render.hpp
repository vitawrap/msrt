#pragma once

#include <vector>
#include "canvas2d_rc.hpp"
#include "resources/ms_tilemap.hpp"

namespace ms {
namespace gfx {

    class TilemapRenderData {
    public:
        // Each layer is a separate draw call
        struct TilemapRenderLayer {
            friend TilemapRenderData;
            
            std::weak_ptr<CanvasDynamicMesh> mesh;
            float uvIncrement;
            short fps, numFrames;

            TilemapRenderLayer() : uvIncrement(0), fps(0), numFrames(1) {}
            void emitQuad(int blockW, int blockH, int x, int y, int atlasWidth, int atlasHeight, int atlasU, int atlasV);
            void submit();
        };

        /** Keep reference to atlas */
        res::ResourceHandle<res::Image> m_atlas;

    private:
        std::vector<TilemapRenderLayer> m_layers;

    public:
        TilemapRenderLayer const* getLayer(unsigned index) const {
            return index < m_layers.size() ? &m_layers[index] : nullptr;
        }

        TilemapRenderLayer* getLayer(unsigned index) {
            return index < m_layers.size() ? &m_layers[index] : nullptr;
        }

        TilemapRenderLayer& addLayer(CanvasRC2D* canvas);

        void clearLayers() {
            m_layers.clear();
        }

        void createFromTilemap(res::TileMap const& tilemap, CanvasRC2D* canvas);

        /** Anchors as defined by map.draw */
        void render(CanvasRC2D* canvas, float x, float y, float w, float h);
    };
    
}
}
