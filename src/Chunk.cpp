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
    siv::PerlinNoise perlinNoise(1234);
    for (int x = 0; x < sizeX; x++) {
        for (int z = 0; z < sizeZ; z++) {
            int worldX = static_cast<int>(position.x) + x;
            int worldZ = static_cast<int>(position.z) + z;

            // 2D noise for surface height
            float noiseValue2D = perlinNoise.octave2D_01(worldX * 0.02f, worldZ * 0.02f, 4, 0.5f);
            int surfaceHeight = static_cast<int>(noiseValue2D * sizeY);

            for (int y = 0; y < sizeY; y++) {
                int worldY = static_cast<int>(position.y) + y;

                // 3D noise for caves
                float noiseValue3D = perlinNoise.octave3D_01(worldX * 0.1f, worldY * 0.1f, worldZ * 0.1f, 4, 0.5f);
                
                // Caves: Create caves based on 3D noise value
                if (y == surfaceHeight - 1) {
                    // The topmost block is grass
                    voxels[x][y][z] = BlockType::Grass;
                } else if (y < surfaceHeight && y >= surfaceHeight - 3) {
                    // The next 3 layers below the surface are dirt
                    voxels[x][y][z] = BlockType::Dirt;
                } else if (y < surfaceHeight - 3) {
                    // Below the dirt is either stone or a cave
                    if (noiseValue3D < 0.4f) {
                        voxels[x][y][z] = BlockType::Air;  // Cave
                    } else {
                        voxels[x][y][z] = BlockType::Stone;  // Stone block
                    }
                } else {
                    // Air above the surface
                    voxels[x][y][z] = BlockType::Air;
                }
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
    // Texture atlas size - 160x160 pixels- each row has 16x16 pixel textures
    float textureSize = 16.0f / 256.0f;  // Each sprite is 16x16 in a 256x256 texture atlas

    int spriteX = 0;  // The x coordinate of the sprite in the texture grid
    int spriteY = 0;  // The y coordinate of the sprite in the texture grid

     if (blockType == BlockType::Grass) {
        if (face == Face::top) {
            spriteX = 0;  // Grass top
            spriteY = 0;  // First row
        } else if (face == Face::bottom) {
            spriteX = 2;  // Dirt block for the bottom
            spriteY = 0;  // First row
        } else if (face == Face::left || face == Face::right || face == Face::front || face == Face::back) {
            spriteX = 3;  // Grass side texture (fourth block in first row)
            spriteY = 0;  // First row
        }
    } else if (blockType == BlockType::Stone) {
        spriteX = 1;  // Stone block
        spriteY = 0;  // First row
    } else if (blockType == BlockType::Dirt) {
        spriteX = 2;  // Dirt block
        spriteY = 0;  // First row
    }

    // Calculate the top-left and bottom-right texture coordinates
    glm::vec2 topLeft = glm::vec2(spriteX * textureSize, 1.0f - (spriteY + 1) * textureSize);
    glm::vec2 bottomRight = glm::vec2((spriteX + 1) * textureSize, 1.0f - spriteY * textureSize);

    if (face == Face::top || face == Face::bottom) {
    texCoords[0] = glm::vec2(topLeft.x, bottomRight.y);      // Bottom-left
    texCoords[1] = glm::vec2(bottomRight.x, bottomRight.y);  // Bottom-right
    texCoords[2] = glm::vec2(bottomRight.x, topLeft.y);      // Top-right
    texCoords[3] = glm::vec2(topLeft.x, topLeft.y);          // Top-left
    }
    // For the other faces, the vertices are defined in the order: bottom-left, top-left, top-right, bottom-right
    // So, we need to adjust the texture coordinates for these faces
    else {
        texCoords[0] = glm::vec2(topLeft.x, bottomRight.y);      // Bottom-left
        texCoords[1] = glm::vec2(topLeft.x, topLeft.y);          // Top-left
        texCoords[2] = glm::vec2(bottomRight.x, topLeft.y);      // Top-right
        texCoords[3] = glm::vec2(bottomRight.x, bottomRight.y);  // Bottom-right
    }


    float voxelVerts[12];
    switch (face) {
        case Face::top:
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y + 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;
        case Face::bottom:
            //Ensure the bottom face texture is upright by swapping texture coordinates as needed

           
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y - 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;
        case Face::right:
            //Ensure the right face texture is upright by swapping texture coordinates as needed
           
            voxelVerts[0] = pos.x + 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x + 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;
        case Face::left:
            // Ensure the left face texture is upright by swapping texture coordinates as needed
          
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x - 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x - 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;
        case Face::front:
            // Ensure the front face texture is upright by swapping texture coordinates as needed
           


            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z + 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z + 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z + 0.5f;
            break;
        case Face::back:

            // Ensure the back face texture is upright by swapping texture coordinates as needed
          
          
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z - 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z - 0.5f;
            break;
    }
    // std::cout << "Voxel position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

    // std::cout << "TexCoords: " << texCoords[0].x << ", " << texCoords[0].y << std::endl;

    
    // Store the vertices and texture coordinates
    vertices.insert(vertices.end(), std::begin(voxelVerts), std::end(voxelVerts));
    // texCoordsArray.insert(texCoordsArray.end(), std::begin(texCoords), std::end(texCoords));
    for (int i = 0; i < 4; i++) {
        texCoordsArray.push_back(texCoords[i].x);  // Add x component (u)
        texCoordsArray.push_back(texCoords[i].y);  // Add y component (v)
    }
    

    
    // Define the face indices (same for all faces)
    unsigned int voxelIndices[6] = {0, 1, 2, 2, 3, 0};
    unsigned int offset = (vertices.size() / 3) - 4;
    if (offset < 0) {
        cout << "offset is less than 0" << endl;
        offset = 0;
    }    
    for (auto index : voxelIndices) {
        indices.push_back(index + offset);
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
        cout << "Voxel type: " << voxels[x][y][z] << endl;
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
    return true;
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
