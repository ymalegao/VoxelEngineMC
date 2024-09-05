#include "Game.hpp"
#include "Cube.hpp"
#include "Chunk.hpp"
#include "Camera.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
using namespace std;

#include <utility>      // For std::pair
#include <functional>   // For std::hash

// Custom hash function for std::pair<int, int>



#define CHUNK_SIZE 16

Chunk *chunk;
Camera *camera;
bool firstMouse = true;
float lastX = 0.0f;
float lastY = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;





// Static member functions need to be defined outside the class
void Game::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Game::mouse_button_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera->processMouseMovement(xoffset, yoffset);
}

void Game::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    std::cout << "Key callback: key=" << key << " action=" << action << std::endl;
    Game* game = (Game*)glfwGetWindowUserPointer(window);
    game->ProcessInput(0.0f);

}


Game::Game(int width, int height) 
    : width(width), height(height) {
}

Game::~Game() {
    glfwTerminate();
}

void Game::Init() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "OpenGL Game", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    std::cout << "Framebuffer width: " << framebufferWidth << " height: " << framebufferHeight << std::endl;
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glfwSetWindowUserPointer(window, this);
    // glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize game objects
    float lastX = framebufferWidth / 2.0f;
    float lastY = framebufferHeight / 2.0f;

    chunk = new Chunk(16,16,16, glm::vec3(0.0f, 0.0f, 0.0f) , this); ;
    camera = new Camera();
    
   
}

void Game::ProcessInput(float deltaTime) {
    
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->processInput(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->processInput(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->processInput(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->processInput(RIGHT, deltaTime);

}

void Game::Update(float deltaTime) {
    UpdateChunks();
}

void Game::UpdateChunks() {
    int playerChunkX = static_cast<int>(camera->cameraPos.x) / CHUNK_SIZE;
    int playerChunkZ = static_cast<int>(camera->cameraPos.z) / CHUNK_SIZE;

    int renderDistance = 3;
    for (int x = playerChunkX - renderDistance; x < playerChunkX + renderDistance; x++){
        for (int z = playerChunkZ - renderDistance; z < playerChunkZ + renderDistance; z++){
            std::pair<int, int> chunkPos = {x, z};

            if (loadedChunks.find(chunkPos) == loadedChunks.end()) {
                loadedChunks[chunkPos] = new Chunk(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, glm::vec3(x * CHUNK_SIZE, 0.0f, z * CHUNK_SIZE) , this);
            }
        }
    }

    for (auto it = loadedChunks.begin(); it != loadedChunks.end();) {
        std::pair<int, int> chunkPos = it->first;

        int x = chunkPos.first;
        int z = chunkPos.second;
        if (x < playerChunkX - renderDistance || x > playerChunkX + renderDistance || z < playerChunkZ - renderDistance || z > playerChunkZ + renderDistance) {
            delete it->second;
            it = loadedChunks.erase(it);
        } else {
            ++it;
        }
    }

}

void Game::Render() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Add this line

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //set color to sky blue
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
    
    //change the view matrix to the camera view matrix

    glm::mat4 view = camera->getViewMatrix();

    // glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
    // rotation += 0.01f;
    // view = glm::rotate(view, rotation, glm::vec3(0.5f, 1.0f, 0.0f));
    
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    // chunk->render(view, projection);
    for (const auto& chunkPair : loadedChunks) {
        cout << "Rendering chunk at " << chunkPair.first.first << " " << chunkPair.first.second << endl;
        chunkPair.second->render(view, projection);
    }


    glfwSwapBuffers(window);
}

void Game::Run() {
    Init();

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        Render();
        Update(deltaTime);
        ProcessInput(deltaTime);
        glfwPollEvents();
        Update(0.0f);
    }
}
