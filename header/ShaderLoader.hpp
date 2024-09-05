#pragma once

#include <string>
#include <glad/glad.h>

class ShaderLoader {
public:
    ShaderLoader(const char* vertex_file_path, const char* fragment_file_path);
    ~ShaderLoader();
    void use();
    GLuint loadShaders(const char* vertex_file_path, const char* fragment_file_path);
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const float* value) const;
    void setVec3(const std::string& name, const float* value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const float* value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    unsigned int ID;
};