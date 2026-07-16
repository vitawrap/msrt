#pragma once

#include "resources/resource.hpp"
#include "resource_manager.hpp"

namespace ms {
namespace res {

    /**
     * Represents a tile map loaded from a microStudio map JSON.
     * NOTE: The Y axis of the tile grid is flipped: (0,0) is the bottom-left corner.
     */
    class TileMap : public Resource {
    public:
        struct Tile {
            unsigned id; // index into m_spriteNames
            short x, y;
        };

        static const Tile EMPTY_TILE;
        
    private:
        std::vector<std::string> m_spriteNames;
        std::vector<Tile> m_tiles;
        
        int m_width, m_height;
        int m_blockWidth, m_blockHeight;
        int* m_tileGrid;

        void addTileInfo(std::string name);
        void allocateGrid();
        
    public:
        TileMap();
        ~TileMap();

        /* Getters */

        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }
        int getBlockWidth() const { return m_blockWidth; }
        int getBlockHeight() const { return m_blockHeight; }

        bool isEmptyTile(Tile tile) const { return tile.id == 0; }
        inline const Tile& getTile(unsigned x, unsigned y) const;
        inline std::string_view getTileName(Tile tile) const;

        DECLARE_LOADER;
    };

    inline const TileMap::Tile& TileMap::getTile(unsigned x, unsigned y) const {
        if (x >= m_width || y >= m_height) return EMPTY_TILE;
        unsigned index = m_tileGrid[x + y * m_width];
        return m_tiles[index];
    }

    std::string_view TileMap::getTileName(Tile tile) const {
        return tile.id >= m_spriteNames.size() ? m_spriteNames[0] : m_spriteNames[tile.id];
    }

}
}
