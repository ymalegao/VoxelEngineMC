#ifndef GAME_HPP
#define GAME_HPP
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

class Game {
public:
    Game(int width, int height);
    ~Game();

    void Run();

private:
    int width, height;
    GLFWwindow* window;
   

    void Init();
    void ProcessInput(float deltaTime);
    void Update(float deltaTime);
    void Render();
    void UpdateChunks();

    // Callbacks
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_button_callback(GLFWwindow* window, double xposIn, double yposIn);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#endif
