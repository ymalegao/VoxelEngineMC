#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Chunk {
public:
    Chunk(int sizeX, int sizeY, int sizeZ, glm::vec3 position);
    ~Chunk();
    void render(const glm::mat4& view, const glm::mat4& projection);
    void generateChunk();
    int sizeX, sizeY, sizeZ;

private:
    unsigned int VAO, VBO, EBO;
    unsigned int shaderProgram;
    void loadShaders(const std::string& vertexPath, const std::string& fragmentPath);
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    void setupMesh();
    glm::vec3 position;
    

};
















#endif