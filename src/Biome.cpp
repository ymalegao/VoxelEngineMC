#include "Biome.hpp"
#include <iostream>
#include <string>


std::map<BiomeType, BiomeProperties> biomeProperties = {
    {BiomeType::Desert,    {0.1f, BlockType::Sand,   BlockType::Sandstone, 0.5f, 0}},
    {BiomeType::Plains,    {0.2f, BlockType::Grass,  BlockType::Dirt,      0.5f, 2}},
    {BiomeType::Forest,    {0.3f, BlockType::Grass,  BlockType::Dirt,      0.5f, 5}},
    {BiomeType::Mountains, {0.5f, BlockType::Stone,  BlockType::Stone,     0.5f, 0}},
};

BiomeType determineBiome(float biomeNoise) {
    if (biomeNoise < -0.3f) {
        return BiomeType::Desert;
    } else if (biomeNoise < 0.0f) {
        return BiomeType::Plains;
    } else if (biomeNoise < 0.3f) {
        return BiomeType::Forest;
    } else {
        return BiomeType::Mountains;
    }
}

bool biomeSupportsTrees(BiomeType biome) {
    return biome == BiomeType::Plains || biome == BiomeType::Forest;
}
