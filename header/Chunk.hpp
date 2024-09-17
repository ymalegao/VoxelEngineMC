#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Game.hpp"
#include "PerlinNoise.hpp"
#include "TexureManager.hpp"



class Game;



enum class BlockType { Air, Grass, Stone, Dirt };

enum Face {
    front,
    back,
    left,
    right,
    top,
    bottom
};

std::ostream& operator<<(std::ostream& os, const BlockType& blockType) {
    switch (blockType) {
        case BlockType::Air: os << "Air"; break;
        // handle other block types...
        default: os << "Unknown block type"; break;
    }
    return os;
}


class Chunk {
public:
    Chunk(int sizeX, int sizeY, int sizeZ, glm::vec3 position, Game *gameRef, GLuint shaderProgram, TextureManager& textureManager);
    ~Chunk();
    TextureManager& textureManager;
    void randomlyRemoveVoxels();
    void render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection);
    void generateChunk();
    void initChunk();
    std::vector<int> tintFlagsArray;
    int sizeX, sizeY, sizeZ;
    std::vector<std::string> faceTextures;
    glm::vec3 position;
    Game *gameRef;
    unsigned int shaderProgram;
    void bindTextures();
    void highlightVoxel(const glm::ivec3& voxel);
    void loadShaders(const std::string& vertexPath, const std::string& fragmentPath);


    bool isVoxelSolid(int x, int y, int z) ;
    std::vector<std::vector<std::vector<BlockType>>> voxels;
    void setupMesh();




private:
    unsigned int VAO, VBO, EBO, CBO, TBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    std::vector<float> texCoordsArray;
    GLuint textureID;

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