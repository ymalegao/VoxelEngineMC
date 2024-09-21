#include "BlockType.hpp"

BlockType getBlockTextureType(BlockType blockType, Face face) {
    switch (blockType) {
        case BlockType::Grass:
            if (face == Face::top)
                return BlockType::GrassTop;
            else if (face == Face::bottom)
                return BlockType::Dirt;
            else
                return BlockType::GrassSide;
        case BlockType::Wood:
            if (face == Face::top || face == Face::bottom)
                return BlockType::WoodTop;
            else
                return BlockType::WoodSide;
        // Handle other special cases if needed
        case BlockType::Sand:
            return BlockType::Sand;
        case BlockType::Stone:
            return BlockType::Stone;
        default:
            return blockType;  // Use the block's own texture
    }
}
