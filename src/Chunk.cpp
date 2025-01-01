#include "Chunk.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

GLenum err;
#define CHECK_GL_ERROR() \
    while ((err = glGetError()) != GL_NO_ERROR) { \
        std::cerr << "OpenGL error: " << err << " at line " << __LINE__ << std::endl; \
    }
    



Chunk::Chunk(int sizeX, int sizeY, int sizeZ, glm::vec3 position , Game *gameRef, GLuint shaderProgram, TextureManager& textureManager) :
    sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ), position(position), gameRef(gameRef), shaderProgram(shaderProgram), textureManager(textureManager) {
    // Load shaders
    // this->sizeX = sizeX;
    // this->sizeY = sizeY;
    // this->sizeZ = sizeZ;

    voxels = std::vector<std::vector<std::vector<BlockType>>>(sizeX, std::vector<std::vector<BlockType>>(sizeY, std::vector<BlockType>(sizeZ)));
    //initialize voxels
    
    // cout << "Creating chunk for sizes" << sizeX << sizeY << sizeX <<  "at position" << position.x << position.y << position.z << endl;
    // loadShaders("VertShader.vertexshader", "FragShader.fragmentshader");
    initChunk();
    generateChunk();
    // setupMesh();
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &TBO);
    glDeleteProgram(shaderProgram);
}

void Chunk::loadShaders(const std::string& vertexPath, const std::string& fragmentPath){
    // Load vertex shader
    std::ifstream vertexFile(vertexPath);
    std::stringstream vertexStream;
    vertexStream << vertexFile.rdbuf();
    std::string vertexCode = vertexStream.str();
    const char* vertexShaderCode = vertexCode.c_str();
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Load fragment shader
    std::ifstream fragmentFile(fragmentPath);
    std::stringstream fragmentStream;
    fragmentStream << fragmentFile.rdbuf();
    std::string fragmentCode = fragmentStream.str();
    const char* fragmentShaderCode = fragmentCode.c_str();
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // Link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // cout << "Loaded shaders" << endl;
}

void Chunk::initializeVoxels(int x, int z, int surfaceY) {
    for (int y = 0; y < sizeY; y++) {
        int worldY = position.y + y;  // Map chunk-local y to world y

        if (worldY == surfaceY) {
            voxels[x][y][z] = BlockType::Grass;  // Surface block
        } else if (worldY < surfaceY && worldY >= surfaceY - 4) {
            voxels[x][y][z] = BlockType::Dirt;  // Subsurface dirt
        } else if (worldY < surfaceY - 4) {
            voxels[x][y][z] = BlockType::Stone;  // Deep stone
        } else {
            voxels[x][y][z] = BlockType::Air;  // Air above surface
        }
    }
}


float Chunk::generateBaseTerrainHeight(int x, int z, const siv::PerlinNoise& perlinNoise) {
    float noiseScale = 0.05f;  // Adjust for terrain frequency
    float maxHeight = sizeY * 0.75f;  // Max height relative to chunk size

    // Perlin noise generates values in [0, 1] → Scale around `position.y`
    float noise = perlinNoise.noise2D_01(x * noiseScale, z * noiseScale);
    float terrainHeight = position.y + (noise * maxHeight);

    return glm::round(terrainHeight);  // Return integer height
}

void Chunk::initChunk() {
    siv::PerlinNoise perlinNoise(1234);

    for (int x = 0; x < sizeX; x++) {
        for (int z = 0; z < sizeZ; z++) {
            int worldX = position.x + x;
            int worldZ = position.z + z;

            // Generate surface height relative to chunk bottom
            int surfaceY = generateBaseTerrainHeight(worldX, worldZ, perlinNoise);

            // Populate voxels in this column
            initializeVoxels(x, z, surfaceY);

            // Place trees only on surface grass blocks
            if (shouldPlaceTree(worldX, worldZ, BiomeType::Plains)) {
                placeTree(x, surfaceY - position.y, z);  // Adjust for chunk-local y
            }
        }
    }
}
bool Chunk::shouldPlaceTree(int worldX, int worldZ, BiomeType biome) {
    int probability = 1; // 10% chance


    // Additional biome-specific checks
    if (biome == BiomeType::Plains || biome == BiomeType::Forest) {
        return (rand() % 100) < probability;
    }

    return false; // Default to no trees in other biomes
}

void Chunk::carveCaves(int x, int z, int minWorldY, int surfaceHeight) {
    int worldX = static_cast<int>(position.x) + x;
    int worldZ = static_cast<int>(position.z) + z;

    for (int y = minWorldY + 1; y <= surfaceHeight; y++) {
        int worldY = y;
        unsigned int seed = 1234;  // Use a consistent seed
        siv::PerlinNoise perlinNoise(seed);



        // **3D Perlin Noise for Caves**
        float caveNoise = perlinNoise.noise3D_01(worldX * 0.02f, worldY * 0.02f, worldZ * 0.02f);

        // **Cave Threshold**
        if (caveNoise > 0.7f) {
            int yIndex = y - minWorldY;
            if (yIndex >= 0 && yIndex < sizeY) {
                voxels[x][yIndex][z] = BlockType::Air;
            }
        }

        // **Additional Noise Layers for Ravines**
        float ravineNoise = perlinNoise.noise2D_01(worldX * 0.01f, worldZ * 0.01f);
        if (ravineNoise > 0.8f && y < surfaceHeight - 5) {
            int yIndex = y - minWorldY;
            if (yIndex >= 0 && yIndex < sizeY) {
                voxels[x][yIndex][z] = BlockType::Air;
            }
        }
    }
}

void Chunk::addSurfaceDetails(int x, int z, int surfaceHeight, BiomeType biome) {
    // **Grass and Flowers**
    if (biome == BiomeType::Plains) {
        if (rand() % 100 < 10) { // 10% chance
            int minWorldY = -sizeY / 2;
            int yIndex = surfaceHeight - minWorldY + 1;
            if (yIndex >= 0 && yIndex < sizeY) {
                voxels[x][yIndex][z] = BlockType::Grass;
            }
        }
    }
    // **Other Biome-Specific Details**
}


void Chunk::generateChunk(){
    vertices.clear();
    indices.clear();
    texCoordsArray.clear();
    
    // cout << "Generating chunk for sizes" << sizeX << sizeX << endl;
    for (int x = 0; x < sizeX; x++){
        for (int z = 0; z < sizeZ; z++){
            for (int y = 0; y < sizeY; y++){
                
                // Only process solid voxels
                if (voxels[x][y][z] != BlockType::Air) {
                    glm::vec3 pos = glm::vec3(x, y, z);

                   
                    // Face culling logic: only add the face if it's adjacent to air or chunk boundary
                    if (y == sizeY - 1 || !isVoxelSolid(x, y + 1, z)) {  // Top face
                        addFace(pos, Face::top);
                    }
                    if (y == 0 || !isVoxelSolid(x, y - 1, z)) {  // Bottom face
                        addFace(pos, Face::bottom);
                    }
                    if (x == sizeX - 1 || !isVoxelSolid(x + 1, y, z)) {  // Right face
                        addFace(pos, Face::right);
                    }
                    if (x == 0 || !isVoxelSolid(x - 1, y, z)) {  // Left face
                        addFace(pos, Face::left);
                    }
                    if (z == sizeZ - 1 || !isVoxelSolid(x, y, z + 1)) {  // Front face
                        addFace(pos, Face::front);
                    }
                    if (z == 0 || !isVoxelSolid(x, y, z - 1)) {  // Back face
                        addFace(pos, Face::back);
                    }
                }
            }
        }
    }
}
void Chunk::addFace(const glm::vec3& pos, Face face) {
    BlockType blockType = voxels[pos.x][pos.y][pos.z];
    glm::vec2 texCoords[4];
    float textureSize = 16.0f / 256.0f;  // Each sprite is 16x16 in a 256x256 texture atlas

    BlockType textureBlockType = getBlockTextureType(blockType, face);
    glm::vec2 spriteCoords = blockTypeToTextureCoords[textureBlockType];
    int spriteX = static_cast<int>(spriteCoords.x);
    int spriteY = static_cast<int>(spriteCoords.y);


    float u0 = spriteX * textureSize;
    float v0 = spriteY * textureSize;
    float u1 = (spriteX + 1) * textureSize;
    float v1 = (spriteY + 1) * textureSize;

    v0 = 1.0f - v0;
    v1 = 1.0f - v1;

    // Calculate texture coordinates
    glm::vec2 bottomLeft = glm::vec2(spriteX * textureSize, spriteY * textureSize);
    glm::vec2 topRight = glm::vec2((spriteX + 1) * textureSize, (spriteY + 1) * textureSize);

    // Explicitly set vertices and texture coordinates for each face
    float voxelVerts[12];
    switch (face) {
        case Face::top:
            // Set UV and vertex positions for the top face
            texCoords[0] = glm::vec2(u0, v1); // 0
            texCoords[1] = glm::vec2(u1, v1); // 1
            texCoords[2] = glm::vec2(u1, v0); // 2
            texCoords[3] = glm::vec2(u0, v0); // 3

            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y + 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;

        case Face::bottom:
            // Set UV and vertex positions for the bottom face
            texCoords[0] = glm::vec2(u0, v1); // 0
            texCoords[1] = glm::vec2(u1, v1); // 1
            texCoords[2] = glm::vec2(u1, v0); // 2
            texCoords[3] = glm::vec2(u0, v0); // 3

            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y - 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;

        case Face::right:
            // Set UV and vertex positions for the right face
            texCoords[0] = glm::vec2(u1, v1); // 0
            texCoords[1] = glm::vec2(u1, v0); // 1
            texCoords[2] = glm::vec2(u0, v0); // 2
            texCoords[3] = glm::vec2(u0, v1); // 3

            voxelVerts[0] = pos.x + 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x + 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;

        case Face::left:
            // Set UV and vertex positions for the left face
            texCoords[0] = glm::vec2(u0, v1); // 0
            texCoords[1] = glm::vec2(u0, v0); // 1
            texCoords[2] = glm::vec2(u1, v0); // 2
            texCoords[3] = glm::vec2(u1, v1); // 3

            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x - 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x - 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;

        case Face::front:
            // Set UV and vertex positions for the front face
            texCoords[0] = glm::vec2(u1, v1); // 0
            texCoords[1] = glm::vec2(u0, v1); // 1
            texCoords[2] = glm::vec2(u0, v0); // 2
            texCoords[3] = glm::vec2(u1, v0); // 3

            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z + 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z + 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;

        case Face::back:
            // Set UV and vertex positions for the back face
            texCoords[0] = glm::vec2(u0, v1); // 0
            texCoords[1] = glm::vec2(u1, v1); // 1
            texCoords[2] = glm::vec2(u1, v0); // 2
            texCoords[3] = glm::vec2(u0, v0); // 3

            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z - 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z - 0.5f;
            break;
    }

    // Store vertices and texture coordinates for the face
    vertices.insert(vertices.end(), std::begin(voxelVerts), std::end(voxelVerts));
    for (int i = 0; i < 4; i++) {
        texCoordsArray.push_back(texCoords[i].x);  // Add u component
        texCoordsArray.push_back(texCoords[i].y);  // Add v component
    }

    // Define the face indices
    unsigned int voxelIndices[6] = {0, 1, 2, 2, 3, 0};
    unsigned int offset = (vertices.size() / 3) - 4;
    for (auto index : voxelIndices) {
        indices.push_back(index + offset);
    }
}


void Chunk::placeTree(int x, int surfaceHeight, int z) {
    if (surfaceHeight < 0 || surfaceHeight >= sizeY - 1) return;

    // Place the trunk
    int trunkHeight = 5 + (rand() % 3);  // Random trunk height
    for (int i = 0; i < trunkHeight; i++) {
        int ny = surfaceHeight + i;
        if (ny >= sizeY) break;
        voxels[x][ny][z] = BlockType::Wood;  // Trunk
    }

    // Create spherical layers for the canopy
    int canopyRadius = 3;  // Radius of the largest layer
    int canopyHeight = 4;  // Height of the canopy
    int canopyCenterY = surfaceHeight + trunkHeight;

    for (int ly = -canopyHeight; ly <= canopyHeight; ly++) {  // Iterate vertically through the spherical canopy
    float layerRadius = canopyRadius * (1.0f - static_cast<float>(std::abs(ly)) / canopyHeight);  // Adjust radius based on height
    int roundedRadius = static_cast<int>(glm::ceil(layerRadius));

    for (int lx = -roundedRadius; lx <= roundedRadius; lx++) {
        for (int lz = -roundedRadius; lz <= roundedRadius; lz++) {
            float distance = glm::sqrt(lx * lx + lz * lz);  // Horizontal distance from the center
            if (distance <= layerRadius) {  // Only include voxels within the current layer's radius
                int nx = x + lx, ny = surfaceHeight + trunkHeight + ly, nz = z + lz;
                if (nx >= 0 && nx < sizeX && ny >= 0 && ny < sizeY && nz >= 0 && nz < sizeZ) {
                    if (voxels[nx][ny][nz] == BlockType::Air) {
                        voxels[nx][ny][nz] = BlockType::Leaves;
                    }
                }
            }
        }
    }
    }
}


void Chunk::bindTextures() {
    for (size_t i = 0; i < faceTextures.size(); ++i) {
        
        GLuint textureID = textureManager.loadTexture(faceTextures[i]);
        if (textureID == 0) {
            std::cerr << "Error: Failed to load texture " << faceTextures[i] << std::endl;
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}


void Chunk::highlightVoxel(const glm::ivec3& voxel) {
    // Ensure the voxel is within the chunk bounds
    cout << "Highlighting voxel" << voxel.x << voxel.y << voxel.z << endl; 
    if (voxel.x >= 0 && voxel.x < sizeX && voxel.y >= 0 && voxel.y < sizeY && voxel.z >= 0 && voxel.z < sizeZ) {
        int faceStartIndex = voxel.x + voxel.y * sizeX + voxel.z * sizeX * sizeY;
        
        // Set the color for the highlighted voxel (e.g., white)
        for (int i = 0; i < 6; i++) {
            colors[faceStartIndex * 18 + i * 3] = 1.0f;      // R
            colors[faceStartIndex * 18 + i * 3 + 1] = 1.0f;  // G
            colors[faceStartIndex * 18 + i * 3 + 2] = 1.0f;  // B
        }
        cout << "done highlighting" << endl;

        
    }
    generateChunk();
    setupMesh();

} 

bool Chunk::isVoxelSolid(int x, int y, int z) {
    if (x >= 0 && x < sizeX && y >= 0 && y < sizeY && z >= 0 && z < sizeZ) {
        //print the voxel type
        
        return voxels[x][y][z] != BlockType::Air;
    }

    //
    if (x == -1) { // Check left neighbor outside of the chunk
        Chunk* leftNeighbor = getLeftNeighbor();
        if (leftNeighbor != nullptr) {
            return leftNeighbor->isVoxelSolid(sizeX - 1, y, z);
        }else{
            return false;
        }

    } else if (x == sizeX) {
        Chunk* rightNeighbor = getRightNeighbor();
        if (rightNeighbor != nullptr) {
            return rightNeighbor->isVoxelSolid(0, y, z);
        }else{
            return false;
        }
    }

    if (y == -1) {
        Chunk* bottomNeighbor = getBottomNeighbor();
        if (bottomNeighbor != nullptr) {
            return bottomNeighbor->isVoxelSolid(x, sizeY - 1, z);
        }
    } else if (y == sizeY) {
        Chunk* topNeighbor = getTopNeighbor();
        if (topNeighbor != nullptr) {
            return topNeighbor->isVoxelSolid(x, 0, z);
        }
    }

    if (z == -1) {
        Chunk* backNeighbor = getBackNeighbor();
        if (backNeighbor != nullptr) {
            return backNeighbor->isVoxelSolid(x, y, sizeZ - 1);
        }
    } else if (z == sizeZ) {
        Chunk* frontNeighbor = getFrontNeighbor();
        if (frontNeighbor != nullptr) {
            return frontNeighbor->isVoxelSolid(x, y, 0);
        }
    }

    // If no neighboring chunk exists, assume non-solid (empty space)
    return false;
}

Chunk* Chunk::getLeftNeighbor() {
    int neighborChunkX = position.x - 1;
    int neighborChunkZ = position.z;
    std::pair<int, int> neighborPos = {neighborChunkX, neighborChunkZ};
    auto it = gameRef->loadedChunks.find(neighborPos); // Access the chunk map from the game

    if (it != gameRef->loadedChunks.end()) {
        return it->second; // Return the chunk pointer if found
    }
    return nullptr; // Return nullptr if the neighbor isn't loaded
}

Chunk* Chunk::getRightNeighbor() {
    int neighborChunkX = position.x + 1;
    int neighborChunkZ = position.z;
    std::pair<int, int> neighborPos = {neighborChunkX, neighborChunkZ};
    auto it = gameRef->loadedChunks.find(neighborPos);

    if (it != gameRef->loadedChunks.end()) {
        return it->second;
    }
    return nullptr;
}

Chunk* Chunk::getFrontNeighbor() {
    int neighborChunkX = position.x;
    int neighborChunkZ = position.z + 1;
    std::pair<int, int> neighborPos = {neighborChunkX, neighborChunkZ};
    auto it = gameRef->loadedChunks.find(neighborPos);

    if (it != gameRef->loadedChunks.end()) {
        return it->second;
    }
    return nullptr;
}

Chunk* Chunk::getBackNeighbor() {
    int neighborChunkX = position.x;
    int neighborChunkZ = position.z - 1;
    std::pair<int, int> neighborPos = {neighborChunkX, neighborChunkZ};
    auto it = gameRef->loadedChunks.find(neighborPos);

    if (it != gameRef->loadedChunks.end()) {
        return it->second;
    }
    return nullptr;
}

Chunk* Chunk::getTopNeighbor() {
    int neighborChunkX = position.x;
    int neighborChunkZ = position.z;
    std::pair<int, int> neighborPos = {neighborChunkX, neighborChunkZ};
    auto it = gameRef->loadedChunks.find(neighborPos);

    if (it != gameRef->loadedChunks.end()) {
        return it->second;
    }
    return nullptr;
}

Chunk* Chunk::getBottomNeighbor() {
    int neighborChunkX = position.x;
    int neighborChunkZ = position.z;
    std::pair<int, int> neighborPos = {neighborChunkX, neighborChunkZ};
    auto it = gameRef->loadedChunks.find(neighborPos);

    if (it != gameRef->loadedChunks.end()) {
        return it->second;
    }
    return nullptr;
}
void Chunk::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &TBO);  // Texture buffer (No need for CBO anymore)

    glBindVertexArray(VAO);

    // Bind vertex buffer (positions)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Bind texture coordinate buffer
    glBindBuffer(GL_ARRAY_BUFFER, TBO);
    glBufferData(GL_ARRAY_BUFFER, texCoordsArray.size() * sizeof(float), texCoordsArray.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);  // 2 components for texture coordinates
    glEnableVertexAttribArray(2);

    
    // Bind element buffer (indices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void Chunk::randomlyRemoveVoxels(){
    int x = rand() % sizeX;
    
    int z = rand() % sizeZ;
    // y height should be from surface, so we start from the top
    for (int y = sizeY - 1; y >= 0; --y) {
        if (voxels[x][y][z] != BlockType::Air) {
            // Remove this voxel
            voxels[x][y][z] = BlockType::Air;
            std::cout << "Removing voxel at " << x << ", " << y << ", " << z << std::endl;

            // Regenerate chunk to reflect the change
            generateChunk();
            setupMesh();
            break;  // Stop after removing one voxel
        }
    }
}



void Chunk::render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(shaderProgram);
    CHECK_GL_ERROR();

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    // Set the uniform matrices (model, view, projection)
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    CHECK_GL_ERROR();

    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    CHECK_GL_ERROR();

    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    CHECK_GL_ERROR();

    glBindVertexArray(VAO);
    CHECK_GL_ERROR();

    // Bind the entire texture atlas
    GLuint textureID = textureManager.loadTexture("pics/mcspritesheet.png");
    if (textureID == 0) {
        std::cerr << "Error: Failed to load texture atlas" << std::endl;
    }
    
    glActiveTexture(GL_TEXTURE0);
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    CHECK_GL_ERROR();  // Check if there's an OpenGL error after binding the texture.
    glUniform1i(glGetUniformLocation(shaderProgram, "blockTexture"), 0);  // Set the atlas to the shader
    CHECK_GL_ERROR();

    // Draw all elements at once using the already set UV coordinates
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    CHECK_GL_ERROR();

    glBindVertexArray(0);
    CHECK_GL_ERROR();
}
