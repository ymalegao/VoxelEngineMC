#ifndef CUBE_HPP
#define CUBE_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Cube {
public:
    Cube();
    ~Cube();
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int VAO, VBO, EBO;
    unsigned int shaderProgram;
    void loadShaders(const std::string& vertexPath, const std::string& fragmentPath);
};

#endif
