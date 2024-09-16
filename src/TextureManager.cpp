#include "TexureManager.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
using namespace std;

GLuint TextureManager::loadTexture(const std::string& texturePath) {
    // std::cout << "Checking texture cache for: " << texturePath << std::endl;
    if (loadedTextures.find(texturePath) != loadedTextures.end()) {
        // std::cout << "Texture already loaded: " << texturePath << " with ID: " << loadedTextures[texturePath] << std::endl;
        return loadedTextures[texturePath];
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    stbi_set_flip_vertically_on_load(true);


    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the sprite sheet as a texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture atlas: " << texturePath << std::endl;
    }
    stbi_image_free(data);
    loadedTextures[texturePath] = textureID;

    
    return textureID;
}

GLuint TextureManager::createTextureFromFile(const std::string& texturePath) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        //rotate the texture 90 degrees
        stbi_set_flip_vertically_on_load(true);
        


        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load the texture image
        int width, height, nrChannels;
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;
            else {
                std::cerr << "Unknown number of channels in texture: " << texturePath << std::endl;
                stbi_image_free(data);
                return 0;
            }

            // Generate texture and mipmaps
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            stbi_image_free(data);
            return 0;
        }

        stbi_image_free(data);
        return textureID;
    }