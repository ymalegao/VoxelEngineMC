#ifndef GAME_HPP
#define GAME_HPP
#include <glad/glad.h>
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Chunk.hpp"
#include <utility>      // For std::pair
#include <functional>   // For std::hash
#include "ShaderLoader.hpp"
#include "ThreadPool.hpp"
#include "TexureManager.hpp"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imGuiHandler.hpp"
#include "Log.hpp"
class Chunk;


struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};


class Game {

public:
    Game(int width, int height);
    ~Game();
    GLuint shaderProgram; 
    ShaderLoader* shaderLoader;
    TextureManager* textureManager;
    GLuint textureID;
    bool raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, Chunk& chunk, glm::ivec3& hitVoxel, float maxDistance);
    void drawRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float length);
    Log logger;
    void Run();
    std::unordered_map<std::pair<int, int>, Chunk*, pair_hash> loadedChunks;

    struct OpenGLState {
    GLint lastProgram;
    GLint lastTexture;
    GLint lastArrayBuffer;
    GLint lastElementArrayBuffer;
    GLint lastVertexArray;
};

void SaveOpenGLState(OpenGLState& state) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &state.lastProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.lastTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.lastArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &state.lastElementArrayBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state.lastVertexArray);
}

void RestoreOpenGLState(const OpenGLState& state) {
    glUseProgram(state.lastProgram);
    glBindTexture(GL_TEXTURE_2D, state.lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, state.lastArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.lastElementArrayBuffer);
    glBindVertexArray(state.lastVertexArray);
}


private:
    int width, height;
    GLFWwindow* window;
    bool castRayForVoxel(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::ivec3& hitVoxel, float maxDistance);
   

    void Init();
    void ProcessInput(float deltaTime);
    void Update(float deltaTime);
    void Render();
    void UpdateChunks();
    GLuint rayVAO, rayVBO;


    // Callbacks
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_button_callback(GLFWwindow* window, double xposIn, double yposIn);
    static void mouse_click_callback(GLFWwindow* window, int button, int action, int mods);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#endif