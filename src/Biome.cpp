#include "Biome.hpp"
#include <iostream>
#include <string>

std::map<BiomeType, BiomeProperties> biomeProperties = {
    // BiomeType        terrainRoughness, maxHeight, surfaceBlock,     subSurfaceBlock, treeProbability
    {BiomeType::Desert,    {0.5f,            15.0f,   BlockType::Sand,    BlockType::Sandstone, 0}},
    {BiomeType::Plains,    {0.6f,            20.0f,   BlockType::Grass,   BlockType::Dirt,      2}},
    {BiomeType::Forest,    {0.7f,            25.0f,   BlockType::Grass,   BlockType::Dirt,      5}},
    {BiomeType::Mountains, {1.0f,            50.0f,   BlockType::Stone,   BlockType::Stone,     0}},
};



BiomeType determineBiome(float biomeNoise) {
    if (biomeNoise < -0.5f) {
        return BiomeType::Desert;
    } else if (biomeNoise < 0.0f) {
        return BiomeType::Plains;
    } else if (biomeNoise < 0.5f) {
        return BiomeType::Forest;
    } else {
        return BiomeType::Mountains;
    }
}

bool biomeSupportsTrees(BiomeType biome) {
    return biomeProperties[biome].treeProbability > 0;
}