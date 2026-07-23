#include "tilemap_render.hpp"
#include "core/app.hpp"
#include "io/log.hpp"
#include "ms/core/view_map.hpp"
#include "browser/classes/player.hpp"

namespace ms {
namespace gfx {

    TilemapRenderData::TilemapRenderLayer& TilemapRenderData::addLayer(CanvasRC2D* canvas)  {
        m_layers.emplace_back();
        TilemapRenderLayer& layer = m_layers.back();
        layer.mesh = canvas->beginDynamicMesh(2048, 2048); // FIXME: triReserve does not work unless it is = to pageSize
        return layer;
    }

    void TilemapRenderData::TilemapRenderLayer::emitQuad
    (int blockW, int blockH, int x, int y, int atlasWidth, int atlasHeight, int atlasU, int atlasV) {
        CanvasTriangle triFirst, triLast;
        float u0 = float(atlasU) / atlasWidth;
        float v0 = float(atlasV) / atlasHeight;
        float u1 = float(atlasU + blockW) / atlasWidth;
        float v1 = float(atlasV + blockH) / atlasHeight;
        x = x * blockW;
        y = y * blockH;
        // First triangle |/
        triFirst.x0 = x; triFirst.y0 = y;
        triFirst.x1 = x + blockW; triFirst.y1 = y;
        triFirst.x2 = x; triFirst.y2 = y + blockH;
        triFirst.u0 = u0; triFirst.v0 = v0;
        triFirst.u1 = u1; triFirst.v1 = v0;
        triFirst.u2 = u0; triFirst.v2 = v1;
        // Last triangle /|
        triLast.x0 = x + blockW; triLast.y0 = y;
        triLast.x1 = x; triLast.y1 = y + blockH;
        triLast.x2 = x + blockW; triLast.y2 = y + blockH;
        triLast.u0 = u1; triLast.v0 = v0;
        triLast.u1 = u0; triLast.v1 = v1;
        triLast.u2 = u1; triLast.v2 = v1;
        CanvasRC2D::addTriangle(mesh, triFirst);
        CanvasRC2D::addTriangle(mesh, triLast);
    }

    void TilemapRenderData::TilemapRenderLayer::submit() {
        CanvasRC2D::endDynamicMesh(mesh);
    }

    /**
     * Populate the render data from the given tilemap.
     */
    void TilemapRenderData::createFromTilemap(res::TileMap const& tilemap, CanvasRC2D* canvas) {
        clearLayers();
        
        auto* project = Application::get()->getProject();
        auto* context = Application::get()->getBrowserContext();
        if (!project || !context) {
            LOG_MSG("Cannot create render data for tilemap without project and context.\n");
            return; // Rendering a tilemap requires project data and spritesheets
        }
        auto* player = context->getPlayer();
        if (!player) {
            LOG_MSG("Cannot create render data for tilemap without Player, is JS initialized?\n");
            return;
        }
        auto* rt = player->getRuntime();
        if (!rt) {
            LOG_MSG("Cannot create render data for tilemap without Runtime, is JS initialized?\n");
            return;
        }

        // start the actual logic
        auto atlas = project->getSpriteAtlas();
        m_atlas = atlas;
        
        auto& defaultLayer = addLayer(canvas);
        ms::unordered_map<TilemapRenderLayer*> animLayers;

        m_blockWidth = tilemap.getBlockWidth();
        m_blockHeight = tilemap.getBlockHeight();
        m_width = tilemap.getWidth();
        m_height = tilemap.getHeight();
        
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                const res::TileMap::Tile& tile = tilemap.getTileYUp(x, y); // TODO: Not cache-friendly
                if (!tilemap.isEmptyTile(tile)) {
                    std::string_view name = tilemap.getTileName(tile);
                    std::string_view path = rt->getSpritePath(name);
                    res::Image::AtlasRect rect;
                    if (atlas->findAtlasRect(path, rect)) {
                        rect.x += tile.x * m_blockWidth;
                        rect.y += tile.y * m_blockHeight;
                        defaultLayer.emitQuad(m_blockWidth, m_blockHeight, x, y, atlas->getWidth(), atlas->getHeight(), rect.x, rect.y);
                    }
                }
            }
        }
        defaultLayer.submit();
    }

    /**
     * Render the data that was built ahead of time.
     */
    void TilemapRenderData::render(CanvasRC2D* canvas, float x, float y, float w, float h) {
        // TODO: is this correct?
        for (auto& layer : m_layers) {
            canvas->setMaterialTexture(m_atlas->toTexture().cast());
            canvas->drawMesh(layer.mesh, x - w*0.5, y - h*0.5, w, h);
        }
    }

}
}
