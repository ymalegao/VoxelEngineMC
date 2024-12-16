// Biome.hpp
#pragma once
#include <map>
#include "BlockType.hpp"  
#include <iostream>
#include <string>

enum class BiomeType {
    Desert,
    Plains,
    Forest,
    Mountains,
};

struct BiomeProperties {
    float terrainRoughness;   // Controls the roughness of the terrain
    float maxHeight;          // Maximum terrain height for the biome
    BlockType surfaceBlock;   // Block type for the surface layer (e.g., grass)
    BlockType subSurfaceBlock; // Block type for the layer beneath the surface (e.g., dirt)
    int treeProbability;      // Probability of tree placement
};

// Declare the biome properties map
extern std::map<BiomeType, BiomeProperties> biomeProperties;

// Function declarations
BiomeType determineBiome(float biomeNoise);
bool biomeSupportsTrees(BiomeType biome);


inline std::ostream& operator<<(std::ostream& os, BiomeType biome) {
    switch (biome) {
        case BiomeType::Desert:
            os << "Desert";
            break;
        case BiomeType::Plains:
            os << "Plains";
            break;
        case BiomeType::Forest:
            os << "Forest";
            break;
        case BiomeType::Mountains:
            os << "Mountains";
            break;
        // Add other cases as needed
        default:
            os << "Unknown Biome";
            break;
    }
    return os;
}
