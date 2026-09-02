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

    TileMap::TileMap(TileMap const& rhs):
        m_width(rhs.m_width), m_height(rhs.m_height),
        m_blockWidth(rhs.m_blockWidth), m_blockHeight(rhs.m_blockHeight),
        m_renderData(nullptr), m_spriteNames(rhs.m_spriteNames), m_tiles(rhs.m_tiles)
    {
        m_tileGrid = new int[m_width * m_height];
        memcpy(m_tileGrid, rhs.m_tileGrid, m_width * m_height * sizeof(int));
    }

    void TileMap::allocateGrid() {
        m_tileGrid = new int[m_width * m_height];
        memset(m_tileGrid, 0, m_width * m_height * sizeof(int));
    }

    void TileMap::addTileInfo(std::string_view name, bool checkDuplicates) {
        short x = 0, y = 0;
        size_t pos = name.rfind(":");
        if (pos != std::string::npos) {
            sscanf(name.data() + pos + 1, "%hd,%hd", &x, &y);
            name = name.substr(0, pos);
        }
        auto it = std::find(m_spriteNames.begin(), m_spriteNames.end(), name);
        unsigned index = std::distance(m_spriteNames.begin(), it);
        if (it == m_spriteNames.end()) {
            m_spriteNames.push_back(std::string{name});
        }
        Tile tile{index, x, y};
        if (!checkDuplicates || std::find(m_tiles.begin(), m_tiles.end(), tile) == m_tiles.end())
            m_tiles.push_back(std::move(tile));
    }

    void TileMap::setTile(std::string_view name, unsigned x, unsigned y) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;
        
        addTileInfo(name, true);
        auto it = std::find_if(m_tiles.begin(), m_tiles.end(), [this, name](Tile const& tile){
            return m_spriteNames[tile.id] == name;
        });
        DEBUG_ASSERT(it != m_tiles.end());
        int idx = std::distance(m_tiles.begin(), it);
        int oldidx = m_tileGrid[(y * m_width) + x];
        if (oldidx != idx) {
            m_tileGrid[(y * m_width) + x] = idx;

            // force graphics rebuild for next draw call
            delete m_renderData;
            m_renderData = nullptr;
        }
    }

    void TileMap::removeTile(unsigned x, unsigned y) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;

        const int idx = 0; // EMPTY_TILE is always the first entry
        int oldidx = m_tileGrid[(y * m_width) + x];
        if (oldidx != idx) {
            m_tileGrid[(y * m_width) + x] = idx;

            // force graphics rebuild for next draw call
            delete m_renderData;
            m_renderData = nullptr;
        }
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
