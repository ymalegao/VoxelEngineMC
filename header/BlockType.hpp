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