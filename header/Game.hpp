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
    bool raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, Chunk& chunk, glm::ivec3& hitVoxel, float maxDistance);
    void drawRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float length);

    void Run();
    std::unordered_map<std::pair<int, int>, Chunk*, pair_hash> loadedChunks;


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