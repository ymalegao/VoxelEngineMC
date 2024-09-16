#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <glad/glad.h>

class TextureManager {
public:
    TextureManager() = default;
    ~TextureManager() = default;

    GLuint loadTexture(const std::string& texturePath);

private:
    std::unordered_map<std::string, GLuint> loadedTextures;  // Cache for loaded textures

    GLuint createTextureFromFile(const std::string& texturePath);
};

#endif // TEXTURE_MANAGER_HPP
