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
            std::weak_ptr<CanvasMesh> mesh;
            float uvBase, uvIncrement;
            short fps, numFrames;

            
        };

    private:
        std::vector<TilemapRenderLayer> m_layers;

    public:
        TilemapRenderLayer const* getLayer(unsigned index) const {
            return index < m_layers.size() ? &m_layers[index] : nullptr;
        }

        TilemapRenderLayer* getLayer(unsigned index) {
            return index < m_layers.size() ? &m_layers[index] : nullptr;
        }

        TilemapRenderLayer& addLayer() {
            m_layers.emplace_back();
            return m_layers.back();
        }

        void clearLayers() {
            m_layers.clear();
        }

        void createFromTilemap(res::TileMap const& tilemap);

        void buildRenderData(CanvasRC2D* canvas);

        void render(CanvasRC2D* canvas);
    };
    
}
}
