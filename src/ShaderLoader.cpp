#include "ShaderLoader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

ShaderLoader::ShaderLoader(const char* vertex_file_path, const char* fragment_file_path) {
    ID = loadShaders(vertex_file_path, fragment_file_path);
}

GLuint ShaderLoader::loadShaders(const char* vertex_file_path, const char* fragment_file_path) {
    // Helper lambda function to read shader files
    auto readFile = [](const char* filePath) -> std::string {
        std::ifstream fileStream(filePath);
        if (!fileStream.is_open()) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        return buffer.str();
    };

    // Read the Vertex and Fragment Shader code
    std::string vertexCode = readFile(vertex_file_path);
    std::string fragmentCode = readFile(fragment_file_path);

    if (vertexCode.empty() || fragmentCode.empty()) {
        return 0;  // Exit if files were not loaded
    }

    // Compile Vertex Shader
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexSourcePointer = vertexCode.c_str();
    glShaderSource(vertexShaderID, 1, &vertexSourcePointer, NULL);
    glCompileShader(vertexShaderID);

    // Check Vertex Shader Compilation
    GLint success;
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorMessage(logLength);
        glGetShaderInfoLog(vertexShaderID, logLength, NULL, errorMessage.data());
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << errorMessage.data() << std::endl;
        return 0;
    }

    // Compile Fragment Shader
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentSourcePointer = fragmentCode.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, NULL);
    glCompileShader(fragmentShaderID);

    // Check Fragment Shader Compilation
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorMessage(logLength);
        glGetShaderInfoLog(fragmentShaderID, logLength, NULL, errorMessage.data());
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << errorMessage.data() << std::endl;
        return 0;
    }

    // Create and link the shader program
    GLuint programID = glCreateProgram();
    if (programID == 0) {
        std::cerr << "ERROR::SHADER::PROGRAM_CREATION_FAILED" << std::endl;
        return 0;
    }

    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    // Check Shader Program Linking
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorMessage(logLength);
        glGetProgramInfoLog(programID, logLength, NULL, errorMessage.data());
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << errorMessage.data() << std::endl;
        return 0;
    }

    // Clean up shaders (they are linked into the program now)
    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programID;
}
