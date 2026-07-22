#include "ms_tilemap.hpp"
#include "io/json.hpp"
#include <algorithm>

#include "graphics/tilemap_render.hpp"

namespace ms {
namespace res {

    const TileMap::Tile TileMap::EMPTY_TILE = { 0, 0, 0 };

    TileMap::TileMap():
        m_width(0), m_height(0), m_blockWidth(0), m_blockHeight(0),
        m_tileGrid(nullptr), m_renderData(nullptr)
    {
        // microStudio uses tile ID 0 for blank tiles
        m_spriteNames.push_back("");
        m_tiles.push_back(EMPTY_TILE);
    }
    
    TileMap::~TileMap() {
        delete m_renderData;
        delete[] m_tileGrid;
    }

    void TileMap::allocateGrid() {
        m_tileGrid = new int[m_width * m_height];
        memset(m_tileGrid, 0, m_width * m_height * sizeof(int));
    }

    void TileMap::addTileInfo(std::string name) {
        short x = 0, y = 0;
        size_t pos = name.rfind(":");
        if (pos != std::string::npos) {
            sscanf(name.c_str() + pos + 1, "%hd,%hd", &x, &y);
            name = name.substr(0, pos);
        }
        auto it = std::find(m_spriteNames.begin(), m_spriteNames.end(), name);
        unsigned index = std::distance(m_spriteNames.begin(), it);
        if (it == m_spriteNames.end()) {
            m_spriteNames.push_back(name);
        }
        m_tiles.push_back({index, x, y});
    }

    Resource* TileMap::loadingHandler(class ms::io::File *file) {
        std::string filename = file->path().string();
        std::string text = file->readString();
        if (text.empty())
            return nullptr;
        
        TileMap* map = new TileMap;
        try {
            io::CJSON json;
            auto root = json.parse(text.c_str());
            
            map->m_width = root["width"].number();
            map->m_height = root["height"].number();
            map->m_blockWidth = root["block_width"].number();
            map->m_blockHeight = root["block_height"].number();

            io::CJSONNode sprites = root["sprites"]; io::CJSONNode sprite;
            int i = 0;
            CJSONNode_forEach(sprite, sprites) {
                if (i++ == 0) continue; // first entry is a zero
                std::string spriteTile = (char const*) sprite;
                map->addTileInfo(spriteTile);
            }
            map->allocateGrid();

            int at = 0;
            io::CJSONNode data = root["data"]; io::CJSONNode datum;
            CJSONNode_forEach(datum, data) {
                int tileId = (double) datum;
                map->m_tileGrid[at++] = tileId;
            }
            
        } catch (io::CJSONError& jsErr) {
            LOG_MSGF("JSON Error: %s when reading out tilemap \"%s\".\n", jsErr.what(), filename.c_str());
            delete map;
            return nullptr;
        }
        return map;
    }

    gfx::TilemapRenderData* TileMap::getRenderData(gfx::CanvasRC2D* canvas) {
        if (!m_renderData) {
            m_renderData = new gfx::TilemapRenderData;
            m_renderData->createFromTilemap(*this, canvas);
        }
        return m_renderData;
    }

}
}
