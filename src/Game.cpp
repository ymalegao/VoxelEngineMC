#include "Game.hpp"
#include "Cube.hpp"
#include "Chunk.hpp"
#include "Camera.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <unordered_set>
using namespace std;

#include <utility>      // For std::pair
#include <functional>   // For std::hash

// Custom hash function for std::pair<int, int>

namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& pair) const {
            return std::hash<int>()(pair.first) ^ std::hash<int>()(pair.second);
        }
    };
}

#define CHUNK_SIZE 16

Chunk *chunk;
Camera *camera;
ShaderLoader *shaderLoader;
bool firstMouse = true;
float lastX = 0.0f;
float lastY = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
ThreadPool threadPool(8);
TextureManager *textureManager = new TextureManager();


std::deque<Chunk*> chunksToAdd;
std::mutex chunkMutex;




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

void Game::mouse_click_callback(GLFWwindow* window, int button, int action, int mods) {
    Game* game = (Game*)glfwGetWindowUserPointer(window);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // for (const auto& chunkPair : game->loadedChunks) {
        //         chunkPair.second->randomlyRemoveVoxels();
      
        
        
        glm::vec3 rayOrigin = camera->cameraPos;         // The origin of the ray is the camera position
        glm::vec3 rayDirection = camera->cameraFront;    // The ray is cast in the direction the camera is facing
        glm::ivec3 hitVoxel;

        cout << "Raycasting" << endl;
        cout << "Ray Origin: " << rayOrigin.x << " " << rayOrigin.y << " " << rayOrigin.z << endl;
        cout << "Ray Direction: " << rayDirection.x << " " << rayDirection.y << " " << rayDirection.z << endl;
        
        if (game->castRayForVoxel(rayOrigin, rayDirection, hitVoxel, 50.0f)) {
        // If a voxel was hit, highlight or mark it (implement the logic to highlight)
            cout << "Voxel hit at " << hitVoxel.x << " " << hitVoxel.y << " " << hitVoxel.z << endl;
            //find the chunk that the ray is in
    }

    }
}


void Game::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    std::cout << "Key callback: key=" << key << " action=" << action << std::endl;
    Game* game = (Game*)glfwGetWindowUserPointer(window);
    game->ProcessInput(0.0f);

}


bool Game::raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, Chunk& chunk, glm::ivec3& hitVoxel, float maxDistance) {
    glm::vec3 rayPos = rayOrigin;  // Starting point of the ray
    glm::vec3 stepSize = glm::vec3(1.0f) / glm::abs(rayDirection);  // Step size for each axis
    glm::ivec3 currentVoxel = glm::ivec3(std::floor(rayPos.x), std::floor(rayPos.y), std::floor(rayPos.z));  // Starting voxel

    glm::ivec3 step = glm::ivec3(rayDirection.x > 0 ? 1 : -1,
                                 rayDirection.y > 0 ? 1 : -1,
                                 rayDirection.z > 0 ? 1 : -1);

    glm::vec3 tMax = (glm::vec3(currentVoxel) + glm::vec3(
        step.x > 0 ? 1.0f : 0.0f, 
        step.y > 0 ? 1.0f : 0.0f, 
        step.z > 0 ? 1.0f : 0.0f) - rayPos) / rayDirection;    
    float distance = 0.0f;

    while (distance < maxDistance) {
        // Debug the current ray position
        std::cout << "Raycasting at voxel: (" << currentVoxel.x << ", " << currentVoxel.y << ", " << currentVoxel.z << ")" << std::endl;
        
        

        // Check if the current voxel is solid
        if (chunk.isVoxelSolid(currentVoxel.x, currentVoxel.y, currentVoxel.z)) {
            hitVoxel = currentVoxel;  // Record the hit voxel
            chunk.voxels[currentVoxel.x][currentVoxel.y][currentVoxel.z] = BlockType::Air;  // Remove the voxel
            chunk.generateChunk();  // Regenerate the chunk
            chunk.setupMesh();  // Setup the mesh
            cout << "we hit a solid voxel" << endl;
            return true;  // Ray hit a solid voxel
        }

        // Move to the next voxel boundary based on tMax
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                currentVoxel.x += step.x;
                distance = tMax.x;
                tMax.x += stepSize.x;
            } else {
                currentVoxel.z += step.z;
                distance = tMax.z;
                tMax.z += stepSize.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                currentVoxel.y += step.y;
                distance = tMax.y;
                tMax.y += stepSize.y;
            } else {
                currentVoxel.z += step.z;
                distance = tMax.z;
                tMax.z += stepSize.z;
            }
        }

        // Break if ray exits chunk bounds
        if (currentVoxel.x < 0 || currentVoxel.x >= chunk.sizeX ||
            currentVoxel.y < 0 || currentVoxel.y >= chunk.sizeY ||
            currentVoxel.z < 0 || currentVoxel.z >= chunk.sizeZ) {
            break;
        }
    }
    cout << "Raycast finished" << endl;
    cout << "Ray hit nothing" << endl;
    return false;  // No voxel was hit
}

Game::Game(int width, int height) 
    : width(width), height(height) {
}

Game::~Game() {
    for (auto& chunkPair : loadedChunks) {
        if (chunkPair.second != nullptr) { // Check if the chunk pointer is valid
            delete chunkPair.second; // Delete each chunk
            chunkPair.second = nullptr; // Avoid dangling pointer
        }
    }
    loadedChunks.clear(); // Clear the map after deletion
    glfwTerminate();       // Terminate GLFW
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
    glEnable(GL_DEPTH_TEST);

    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, mouse_click_callback);
    glfwSetCursorPosCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);

    // Allocate space for 2 vertices (start and end of the line)
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr, GL_DYNAMIC_DRAW);

    // Specify vertex attribute pointer for position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Initialize game objects
    float lastX = framebufferWidth / 2.0f;
    float lastY = framebufferHeight / 2.0f;

    // chunk = new Chunk(16,16,16, glm::vec3(0.0f, 0.0f, 0.0f) , this); ;
    camera = new Camera();
    shaderProgram = shaderLoader->loadShaders("VertShader.vertexshader", "FragShader.fragmentshader");    
    this->textureManager = new TextureManager();
    this->textureID = textureManager->loadTexture("pics/spritesheet.png");
    cout << "Texture ID: " << textureID << endl;
}
void Game::drawRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float length) {
    glm::vec3 rayEnd = rayOrigin + rayDirection * length;

    // Update the VBO with new vertex data (ray origin and ray end)
    glm::vec3 vertices[] = { rayOrigin, rayEnd };
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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
std::unordered_set<std::pair<int, int>> chunksInQueue;


void Game::UpdateChunks() {
    int playerChunkX = static_cast<int>(camera->cameraPos.x) / CHUNK_SIZE;
    int playerChunkZ = static_cast<int>(camera->cameraPos.z) / CHUNK_SIZE;

    int renderDistance = 3;
    for (int x = playerChunkX - renderDistance; x < playerChunkX + renderDistance; x++){
        for (int z = playerChunkZ - renderDistance; z < playerChunkZ + renderDistance; z++){
            std::pair<int, int> chunkPos = {x, z};

            if (loadedChunks.find(chunkPos) == loadedChunks.end() && chunksInQueue.find(chunkPos) == chunksInQueue.end()) {
                // Only enqueue if the chunk is neither loaded nor already in the queue
                std::cout << "Enqueueing new chunk at: (" << x << ", " << z << ")" << std::endl;
                chunksInQueue.insert(chunkPos); // Mark chunk as enqueued
                
                threadPool.enqueueTask([this, chunkPos, x, z]() {
                    Chunk* newChunk = new Chunk(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, glm::vec3(x * CHUNK_SIZE, 0.0f, z * CHUNK_SIZE), this, shaderProgram, *textureManager);
                    
                    std::lock_guard<std::mutex> lock(chunkMutex);
                    chunksToAdd.push_back(newChunk);

                    chunksInQueue.erase(chunkPos); // Remove from the queue once processed
                });
            }
        }
    }

    std::lock_guard<std::mutex> lock(chunkMutex);
    while (!chunksToAdd.empty()) {


        Chunk* newChunk = chunksToAdd.front();
        chunksToAdd.pop_front();
        newChunk->setupMesh();
        loadedChunks[{static_cast<int>(newChunk->position.x / CHUNK_SIZE), static_cast<int>(newChunk->position.z / CHUNK_SIZE)}] = newChunk;
        cout << "Loaded chunk at " << newChunk->position.x << " " << newChunk->position.z << endl;
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


bool Game::castRayForVoxel(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::ivec3& hitVoxel, float maxDistance) {
    //find the chunk that the ray is in
    for (const auto& chunkPair : loadedChunks) {
        if (chunkPair.second->position.x <= rayOrigin.x && rayOrigin.x < chunkPair.second->position.x + CHUNK_SIZE &&
            chunkPair.second->position.z <= rayOrigin.z && rayOrigin.z < chunkPair.second->position.z + CHUNK_SIZE) {
            cout << "Ray is in chunk at " << chunkPair.first.first << " " << chunkPair.first.second << endl;
            return raycast(rayOrigin, rayDirection, *chunkPair.second, hitVoxel, maxDistance);
        }
    }
    
}
void Game::Render() {
    // Enable wireframe mode for debugging (if needed)
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set the sky color to light sky blue
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

    // Set up lighting and ambient colors
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));  // Direction of the light
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);  // White light (corrected)
    glm::vec3 ambientColor = glm::vec3(0.53f, 0.81f, 0.98f);  // Light sky blue as ambient

    glUseProgram(shaderProgram);

    // Pass light information to the shader
    GLuint lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint ambientColorLoc = glGetUniformLocation(shaderProgram, "ambientColor");

    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(ambientColorLoc, 1, glm::value_ptr(ambientColor));



    // Bind the texture atlas once (assuming you have the texture atlas loaded in texture unit 0)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureManager->loadTexture("pics/mcspritesheet.png"));
    GLint textureUniformLoc = glGetUniformLocation(shaderProgram, "blockTexture");
    glUniform1i(textureUniformLoc, 0);  // Texture unit 0

    // Update the view matrix from the camera
    glm::mat4 view = camera->getViewMatrix();
    
    // Set the projection matrix for 3D perspective
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    // Render all chunks
    for (const auto& chunkPair : loadedChunks) {
        chunkPair.second->render(shaderProgram, view, projection);
    }

    // Swap buffers to display the rendered frame
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
    }
}