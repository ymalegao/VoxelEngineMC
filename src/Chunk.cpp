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
void Chunk::initChunk() {
    unsigned int seed = 1234;  // Use a consistent seed
    siv::PerlinNoise perlinNoise(seed);
    int maxHeight = sizeY*0.5;

    for (int x = 0; x < sizeX; x++) {
        for (int z = 0; z < sizeZ; z++) {
            int worldX = static_cast<int>(position.x) + x;
            int worldZ = static_cast<int>(position.z) + z;

            // Biome noise calculation
            float biomeFrequency = 0.02f;
            float biomeAmplitude = 1.0f;
            float biomePersistence = 0.5f;
            int biomeOctaves = 4;
            float biomeNoise = 0.0f;
            float maxBiomeAmplitude = 0.0f;
            float currentBiomeFrequency = biomeFrequency;
            float currentBiomeAmplitude = biomeAmplitude;

            for (int i = 0; i < biomeOctaves; i++) {
                biomeNoise += perlinNoise.noise2D_01(worldX * currentBiomeFrequency, worldZ * currentBiomeFrequency) * currentBiomeAmplitude;
                maxBiomeAmplitude += currentBiomeAmplitude;
                currentBiomeAmplitude *= biomePersistence;
                currentBiomeFrequency *= 2.0f;
            }

            biomeNoise /= maxBiomeAmplitude; // Normalize to [0, 1]
            biomeNoise = biomeNoise * 2.0f - 1.0f; // Map to [-1, 1]

            BiomeType biome = determineBiome(biomeNoise);
            BiomeProperties properties = biomeProperties[biome];

            // Terrain height calculation
            float frequency = 0.01f;
            float amplitude = 80.0f * properties.terrainRoughness;  // Increased amplitude
            float terrainNoise = 0.0f;
            float persistence = 0.5f;
            int octaves = 4;

            float currentAmplitude = amplitude;
            float currentFrequency = frequency;

            for (int i = 0; i < octaves; i++) {
                terrainNoise += perlinNoise.noise2D_01(worldX * currentFrequency, worldZ * currentFrequency) * currentAmplitude;
                currentAmplitude *= persistence;
                currentFrequency *= 2.0f;
            }

            terrainNoise = glm::clamp(terrainNoise, 0.0f, (float)(maxHeight - 1));
            int surfaceHeight = static_cast<int>(terrainNoise);

            // Initialize all voxels to Air
            for (int y = 0; y < sizeY; y++) {
                voxels[x][y][z] = BlockType::Air;
            }

            // Fill solid blocks up to surfaceHeight
            for (int y = 0; y <= surfaceHeight; y++) {
                if (y == surfaceHeight) {
                    voxels[x][y][z] = properties.surfaceBlock;
                } else {
                    voxels[x][y][z] = properties.undergroundBlock;
                }
            }

            // Carve caves using 3D noise
            for (int y = 1; y <= surfaceHeight; y++) {  // Start from y=1 to avoid caves on the surface
                int worldY = static_cast<int>(position.y) + y;
                float caveFrequency = 0.05f;
                float caveThreshold = 0.6f; // Adjust as needed
                float caveNoise = perlinNoise.noise3D_01(worldX * caveFrequency, worldY * caveFrequency, worldZ * caveFrequency);

                if (caveNoise > caveThreshold) {
                    // Create cave
                    voxels[x][y][z] = BlockType::Air;
                }
            }

            // Tree placement
            if (biomeSupportsTrees(biome) && rand() % 100 < properties.treeProbability) {
                placeTree(x, surfaceHeight + 1, z);
            }
        }
    }
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


void Chunk::placeTree(int x, int y, int z) {
    int trunkHeight = 5;  // You can randomize this if desired

    // Trunk
    for (int i = 0; i < trunkHeight; i++) {
        if (y + i < sizeY) {
            voxels[x][y + i][z] = BlockType::Wood;
        }
    }

    // Leaves (simple cube around the top of the trunk)
    for (int lx = -2; lx <= 2; lx++) {
        for (int ly = trunkHeight - 2; ly <= trunkHeight + 2; ly++) {
            for (int lz = -2; lz <= 2; lz++) {
                int nx = x + lx;
                int ny = y + ly;
                int nz = z + lz;

                if (nx >= 0 && nx < sizeX && ny >= 0 && ny < sizeY && nz >= 0 && nz < sizeZ) {
                    // Simple spherical shape condition
                    if (lx * lx + ly * ly + lz * lz <= 3 * 3) {
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
