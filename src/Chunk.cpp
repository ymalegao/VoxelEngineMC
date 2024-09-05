#include "Chunk.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include "PerlinNoise.hpp"
using namespace std;

Chunk::Chunk(int sizeX, int sizeY, int sizeZ, glm::vec3 position) :
    sizeX(sizeX), sizeY(sizeY), sizeZ(sizeZ), position(position) {
    // Load shaders
    // this->sizeX = sizeX;
    // this->sizeY = sizeY;
    // this->sizeZ = sizeZ;
    cout << "Creating chunk for sizes" << sizeX << sizeY << sizeX <<  "at position" << position.x << position.y << position.z << endl;
    loadShaders("VertShader.vertexshader", "FragShader.fragmentshader");
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

    cout << "Loaded shaders" << endl;
}



void Chunk::generateChunk(){
    
    cout << "Generating chunk for sizes" << sizeX << sizeX << endl;
    siv::PerlinNoise perlinNoise(1234);
    for (int x = 0; x < sizeX; x++){
        for (int z = 0; z < sizeZ; z++){
            
            int worldX = static_cast<int>(position.x) + x;
            int worldZ = static_cast<int>(position.z) + z;
            
            float noiseValue = perlinNoise.noise2D_01(worldX * 0.15f, worldZ * 0.1f); // Adjust the scale for noise as needed

            int height = static_cast<int>(noiseValue * sizeY);

            for (int y = 0; y < height; y++){
                glm::vec3 pos = glm::vec3(x, y, z);
                float voxelVertices[] = {
                    pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f,
                    pos.x + 0.5f, pos.y - 0.5f, pos.z - 0.5f,
                    pos.x + 0.5f, pos.y + 0.5f, pos.z - 0.5f,
                    pos.x - 0.5f, pos.y + 0.5f, pos.z - 0.5f,
                    pos.x - 0.5f, pos.y - 0.5f, pos.z + 0.5f,
                    pos.x + 0.5f, pos.y - 0.5f, pos.z + 0.5f,
                    pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f,
                    pos.x - 0.5f, pos.y + 0.5f, pos.z + 0.5f
                };

                unsigned int voxelIndices[] = {
                    0, 1, 2, 2, 3, 0,
                    4, 5, 6, 6, 7, 4,
                    0, 1, 5, 5, 4, 0,
                    2, 3, 7, 7, 6, 2,
                    0, 3, 7, 7, 4, 0,
                    1, 2, 6, 6, 5, 1
                };

                vertices.insert(vertices.end(), std::begin(voxelVertices), std::end(voxelVertices));

                unsigned int offset = vertices.size() / 3 - 8; // this math is becasue you have 24 vertices in the cube and then 24/3 = 8 - 8 is the index of the first vertex of the cube
                for (auto i : voxelIndices) {
                    indices.push_back(i + offset);
                }
            }
        }
                    
    }
    cout << "Generated chunk" << endl;
    cout << "Vertices size: " << vertices.size() << endl;
    cout << "Indices size: " << indices.size() << endl;

}

void Chunk::setupMesh(){
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    // Bind the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Bind the element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute (corrected to match your vertex data layout)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Chunk::render(const glm::mat4& view, const glm::mat4& projection) {
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