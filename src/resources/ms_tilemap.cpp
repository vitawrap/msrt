#include "ms_tilemap.hpp"
#include "io/json.hpp"
#include <algorithm>

namespace ms {
namespace res {

    const TileMap::Tile TileMap::EMPTY_TILE = { 0, 0, 0 };

    TileMap::TileMap():
        m_width(0), m_height(0), m_blockWidth(0), m_blockHeight(0),
        m_tileGrid(nullptr)
    {
        // microStudio uses tile ID 0 for blank tiles
        m_spriteNames.push_back("");
    }
    
    TileMap::~TileMap() {
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
        if (it == m_spriteNames.end()) {
            m_spriteNames.push_back(name);
        }
        unsigned index = it - m_spriteNames.begin();
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
            
            map->m_width = root["width"];
            map->m_height = root["height"];
            map->m_blockWidth = root["block_width"];
            map->m_blockHeight = root["block_height"];

            auto sprites = root["sprites"]; io::CJSONNode sprite;
            CJSONNode_forEach(sprite, sprites) {
                std::string spriteTile = (char const*) sprite;
                map->addTileInfo(spriteTile);
            }
            map->allocateGrid();

            int at = 0;
            auto data = root["data"]; io::CJSONNode datum;
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

}
}
