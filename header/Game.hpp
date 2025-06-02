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
        GLint lastActiveTexture;
        GLint lastArrayBuffer;
        GLint lastElementArrayBuffer;
        GLint lastVertexArray;
        GLint lastViewport[4];
        GLint lastScissorBox[4];
        bool blendEnabled;
        bool depthTestEnabled;
        bool cullFaceEnabled;
        bool scissorTestEnabled;
        GLint blendSrcAlpha;
        GLint blendDstAlpha;
        GLint blendSrcRgb;
        GLint blendDstRgb;
        GLint depthFunc;
        GLboolean colorMask[4];
        GLfloat lineWidth;
        GLint polygonMode[2];
    };


void SaveOpenGLState(OpenGLState& state) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &state.lastProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.lastTexture);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &state.lastActiveTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.lastArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &state.lastElementArrayBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state.lastVertexArray);
    glGetIntegerv(GL_VIEWPORT, state.lastViewport);
    glGetIntegerv(GL_SCISSOR_BOX, state.lastScissorBox);

    state.blendEnabled = glIsEnabled(GL_BLEND);
    state.depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    state.cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    state.scissorTestEnabled = glIsEnabled(GL_SCISSOR_TEST);
    
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &state.blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &state.blendDstAlpha);
    glGetIntegerv(GL_BLEND_SRC_RGB, &state.blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &state.blendDstRgb);
    glGetIntegerv(GL_DEPTH_FUNC, &state.depthFunc);
    glGetBooleanv(GL_COLOR_WRITEMASK, state.colorMask);
    glGetFloatv(GL_LINE_WIDTH, &state.lineWidth);
    glGetIntegerv(GL_POLYGON_MODE, state.polygonMode);
}

void RestoreOpenGLState(const OpenGLState& state) {
    glUseProgram(state.lastProgram);
    glActiveTexture(state.lastActiveTexture);
    glBindTexture(GL_TEXTURE_2D, state.lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, state.lastArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.lastElementArrayBuffer);
    glBindVertexArray(state.lastVertexArray);
    glViewport(state.lastViewport[0], state.lastViewport[1], state.lastViewport[2], state.lastViewport[3]);
    glScissor(state.lastScissorBox[0], state.lastScissorBox[1], state.lastScissorBox[2], state.lastScissorBox[3]);

    if (state.blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (state.depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (state.cullFaceEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (state.scissorTestEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    
    glBlendFuncSeparate(state.blendSrcRgb, state.blendDstRgb, state.blendSrcAlpha, state.blendDstAlpha);
    glDepthFunc(state.depthFunc);
    glColorMask(state.colorMask[0], state.colorMask[1], state.colorMask[2], state.colorMask[3]);
    glLineWidth(state.lineWidth);
    glPolygonMode(GL_FRONT_AND_BACK, state.polygonMode[0]);
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
