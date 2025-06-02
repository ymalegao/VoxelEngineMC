#pragma once
#ifndef GL_DEBUG_HPP
#define GL_DEBUG_HPP

#include <glad/glad.h>
#include <iostream>
#include <string>
#include <map>

namespace GLDebug {

    // Error code to string mapping
    inline std::string GetErrorString(GLenum error) {
        static const std::map<GLenum, std::string> errorMap = {
            { GL_NO_ERROR, "GL_NO_ERROR" },
            { GL_INVALID_ENUM, "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument" },
            { GL_INVALID_VALUE, "GL_INVALID_VALUE: A numeric argument is out of range" },
            { GL_INVALID_OPERATION, "GL_INVALID_OPERATION: The specified operation is not allowed in the current state" },
            { GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete" },
            { GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command" },
            { GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW: An attempt has been made to perform an operation that would cause an internal stack to underflow" },
            { GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW: An attempt has been made to perform an operation that would cause an internal stack to overflow" }
        };

        auto it = errorMap.find(error);
        if (it != errorMap.end()) {
            return it->second;
        } else {
            return "Unknown Error: " + std::to_string(error);
        }
    }

    // Check for OpenGL errors and print detailed information
    inline bool CheckError(const char* file, int line, const char* function) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL Error at " << file << ":" << line << " in " << function << ": "
                      << GetErrorString(error) << std::endl;
            return false;
        }
        return true;
    }

    // Check uniform location and print diagnostic information
    inline bool CheckUniform(GLint location, const std::string& uniformName, const char* file, int line) {
        if (location == -1) {
            std::cerr << "Uniform '" << uniformName << "' not found in shader program. "
                      << "Location: " << file << ":" << line << std::endl;
            return false;
        }
        return true;
    }

    // Check if shader program is valid
    inline bool CheckShaderProgram(GLuint program, const char* file, int line) {
        if (program == 0) {
            std::cerr << "Invalid shader program (0) at " << file << ":" << line << std::endl;
            return false;
        }
        
        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
            
            std::cerr << "Shader program linking failed at " << file << ":" << line << ":\n"
                      << &infoLog[0] << std::endl;
            return false;
        }
        
        return true;
    }

    // Check if texture is valid
    inline bool CheckTexture(GLuint texture, const std::string& textureName, const char* file, int line) {
        if (texture == 0) {
            std::cerr << "Invalid texture ID (0) for '" << textureName << "' at " << file << ":" << line << std::endl;
            return false;
        }
        return true;
    }

    // Check for validation errors in the current state
    inline bool ValidateState(const char* file, int line) {
        glValidateProgram(glGetInteger(GL_CURRENT_PROGRAM));
        GLint validateStatus = 0;
        glGetProgramiv(glGetInteger(GL_CURRENT_PROGRAM), GL_VALIDATE_STATUS, &validateStatus);
        
        if (validateStatus == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(glGetInteger(GL_CURRENT_PROGRAM), GL_INFO_LOG_LENGTH, &maxLength);
            
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(glGetInteger(GL_CURRENT_PROGRAM), maxLength, &maxLength, &infoLog[0]);
            
            std::cerr << "Program validation failed at " << file << ":" << line << ":\n"
                      << &infoLog[0] << std::endl;
            return false;
        }
        return true;
    }
}

// Convenience macros
#define GL_CHECK() GLDebug::CheckError(__FILE__, __LINE__, __FUNCTION__)
#define GL_CHECK_UNIFORM(location, name) GLDebug::CheckUniform(location, name, __FILE__, __LINE__)
#define GL_CHECK_PROGRAM(program) GLDebug::CheckShaderProgram(program, __FILE__, __LINE__)
#define GL_CHECK_TEXTURE(texture, name) GLDebug::CheckTexture(texture, name, __FILE__, __LINE__)
#define GL_VALIDATE_STATE() GLDebug::ValidateState(__FILE__, __LINE__)

#endif // GL_DEBUG_HPP