#include "Chunk.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;



Chunk::Chunk(int sizeX, int sizeY, int sizeZ, glm::vec3 position , Game *gameRef) :
    sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ), position(position), gameRef(gameRef) {
    // Load shaders
    // this->sizeX = sizeX;
    // this->sizeY = sizeY;
    // this->sizeZ = sizeZ;
    voxels = std::vector<std::vector<std::vector<bool>>>(sizeX, std::vector<std::vector<bool>>(sizeY, std::vector<bool>(sizeZ)));
    //initialize voxels
    

    // cout << "Creating chunk for sizes" << sizeX << sizeY << sizeX <<  "at position" << position.x << position.y << position.z << endl;
    loadShaders("VertShader.vertexshader", "FragShader.fragmentshader");
    initChunk();
    generateChunk();
    setupMesh();
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
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
                if (noiseValue3D < 0.4f && y < surfaceHeight) {
                    voxels[x][y][z] = false; // Create a cave
                } else if (y <= surfaceHeight) {
                    voxels[x][y][z] = true;  // Solid ground
                } else {
                    voxels[x][y][z] = false; // Air above the ground
                }
            }
        }
    }
}


void Chunk::generateChunk(){
    vertices.clear();
    indices.clear();
    colors.clear();
    
    // cout << "Generating chunk for sizes" << sizeX << sizeX << endl;
    for (int x = 0; x < sizeX; x++){
        for (int z = 0; z < sizeZ; z++){
            for (int y = 0; y < sizeY; y++){
                // Only process solid voxels
                if (voxels[x][y][z]) {
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


void Chunk::addFace(const glm::vec3&pos , Face face){
    float voxelVerts[12]; // 4 vertices * 3 coordinates
    unsigned int voxelIndices[6] = {0, 1, 2, 2, 3, 0}; // 2 triangles for each face
    glm::vec3 faceColor;
    float faceColors[12]; // 4 vertices * 3 color components (R, G, B)


    switch (face){
        case Face::top:
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y + 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z + 0.5f;
            faceColor = glm::vec3(1.0f, 0.0f, 0.0f);  // Red for top

            
            break;
        case Face::bottom:
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y - 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            faceColor = glm::vec3(0.0f, 1.0f, 0.0f);  // Green for bottom
            
            break;
        case Face::right:
            voxelVerts[0] = pos.x + 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x + 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            faceColor = glm::vec3(0.0f, 0.0f, 1.0f);  // Blue for right
            
            break;
        case Face::left:
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x - 0.5f; voxelVerts[4] = pos.y + 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x - 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y - 0.5f; voxelVerts[11] = pos.z + 0.5f;
            faceColor = glm::vec3(1.0f, 1.0f, 0.0f);  // Yellow for left
            
            break;
        case Face::front:
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z + 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z + 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z + 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z + 0.5f;
            faceColor = glm::vec3(1.0f, 0.0f, 1.0f);  // Magenta for front
            
            break;
        case Face::back:
            voxelVerts[0] = pos.x - 0.5f; voxelVerts[1] = pos.y - 0.5f; voxelVerts[2] = pos.z - 0.5f;
            voxelVerts[3] = pos.x + 0.5f; voxelVerts[4] = pos.y - 0.5f; voxelVerts[5] = pos.z - 0.5f;
            voxelVerts[6] = pos.x + 0.5f; voxelVerts[7] = pos.y + 0.5f; voxelVerts[8] = pos.z - 0.5f;
            voxelVerts[9] = pos.x - 0.5f; voxelVerts[10] = pos.y + 0.5f; voxelVerts[11] = pos.z - 0.5f;
            faceColor = glm::vec3(0.0f, 1.0f, 1.0f);  // Cyan for back
            
            break;

    }

    for (int i = 0; i < 4; i++) {
        faceColors[i * 3] = faceColor.r;
        faceColors[i * 3 + 1] = faceColor.g;
        faceColors[i * 3 + 2] = faceColor.b;
    }

    colors.insert(colors.end(), std::begin(faceColors), std::end(faceColors)); // Add the face's colors for each vertex


    vertices.insert(vertices.end(), std::begin(voxelVerts), std::end(voxelVerts));

    unsigned int offset = vertices.size() / 3 - 4;
    for (auto index : voxelIndices){
        indices.push_back(index + offset);
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
        int worldX = static_cast<int>(position.x) + x;
        int worldZ = static_cast<int>(position.z) + z;

        // Generate height using noise (adjust the scale and parameters as needed)
        siv::PerlinNoise perlinNoise(1234);  // Seed for consistency across chunks
        float noiseValue = perlinNoise.octave2D_01(worldX * 0.02f, worldZ * 0.02f, 4, 0.5f);
        int height = static_cast<int>(noiseValue * sizeY);
        // voxels[x][y][z] = (y <= height - 1);
        // Check if the y position is below or at the generated height or if it's the top layer 
        // cout << "Checking voxel" << x << y << z << endl;
        // cout << "voxel is " << voxels[x][y][z] << endl;
        return voxels[x][y][z];
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

void Chunk::setupMesh(){
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &CBO); // Color buffer

    glBindVertexArray(VAO);
    
    // Bind the vertex buffer (positions)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Vertex Position Attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Bind the color buffer (colors)
    glBindBuffer(GL_ARRAY_BUFFER, CBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);

    // Vertex Color Attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Bind the element buffer (indices)
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
        if (voxels[x][y][z]) {
            // Remove this voxel
            voxels[x][y][z] = false;
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

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);


    
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    



    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}