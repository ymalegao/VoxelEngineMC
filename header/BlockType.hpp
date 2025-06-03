#pragma once
#ifndef BLOCKTYPE_HPP
#define BLOCKTYPE_HPP
#include <unordered_map>
#include <glm/glm.hpp>
#include <stdio.h>

enum Face {
    front,
    back,
    left,
    right,
    top,
    bottom
};



enum class BlockType { Air, Grass, Wood, GrassSide, Stone, Dirt, Sand, WoodSide, GrassTop, WoodTop , Sandstone, Leaves};

inline std::string blockTypeToString(BlockType type) {
    switch (type) {
        case BlockType::Air:        return "Air";
        case BlockType::Grass:      return "Grass";
        case BlockType::GrassSide:  return "GrassSide";
        case BlockType::GrassTop:   return "GrassTop";
        case BlockType::Stone:      return "Stone";
        case BlockType::Dirt:       return "Dirt";
        case BlockType::Sand:       return "Sand";
        case BlockType::Wood:       return "Wood";
        case BlockType::WoodSide:   return "WoodSide";
        case BlockType::WoodTop:    return "WoodTop";
        case BlockType::Sandstone:  return "Sandstone";
        case BlockType::Leaves:     return "Leaves";
        default:                    return "Unknown";
    }
}

static std::unordered_map<BlockType, glm::vec2> blockTypeToTextureCoords = {
    {BlockType::GrassSide, {3,0}},
    {BlockType::GrassTop, {0, 0}},
    {BlockType::Stone, {1, 0}},
    {BlockType::Dirt, {2, 0}},
    {BlockType::Sand, {2, 1}},
    {BlockType::WoodSide, {4, 1}},
    {BlockType::WoodTop, {5, 1}},
    {BlockType::Sandstone, {0, 12}},
    {BlockType::Leaves, {1,9}}
};

BlockType getBlockTextureType(BlockType blockType, Face face);

#endif // BLOCKTYPE_HPP
