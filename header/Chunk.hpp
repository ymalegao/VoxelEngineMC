#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Game.hpp"
#include "PerlinNoise.hpp"


class Game;

enum Face {
    front,
    back,
    left,
    right,
    top,
    bottom
};


class Chunk {
public:
    Chunk(int sizeX, int sizeY, int sizeZ, glm::vec3 position, Game *gameRef);
    ~Chunk();
    void randomlyRemoveVoxels();
    void render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
    void generateChunk();
    void initChunk();
    int sizeX, sizeY, sizeZ;
    glm::vec3 position;
    Game *gameRef;
    unsigned int shaderProgram;
    void highlightVoxel(const glm::ivec3& voxel);

    bool isVoxelSolid(int x, int y, int z) ;
    std::vector<std::vector<std::vector<bool>>> voxels; // 3D vector to store voxel states (true = solid, false = air)
    void setupMesh();




private:
    unsigned int VAO, VBO, EBO, CBO;
    void loadShaders(const std::string& vertexPath, const std::string& fragmentPath);
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    std::vector<float> colors;
    void addFace(const glm::vec3& pos, Face face);
    Chunk* getLeftNeighbor();
    Chunk* getRightNeighbor();
    Chunk* getFrontNeighbor();
    Chunk* getBackNeighbor();
    Chunk* getTopNeighbor();
    Chunk* getBottomNeighbor();

    

};
















#endif