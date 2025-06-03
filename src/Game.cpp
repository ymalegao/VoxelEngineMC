#include "Game.hpp"
#include "Cube.hpp"
#include "Chunk.hpp"
#include "Camera.hpp"
#include <cmath>
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


#define PRINT_BLOCK_TYPE(bt) \
    std::cout << "This voxel is a " << blockTypeToString(bt) << std::endl;


#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 32

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
bool isInteractingWithImGui = false;

std::deque<Chunk*> chunksToAdd;
std::mutex chunkMutex;
Log logger;

float outlineVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,  -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f,  0.5f, -0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f
    };

    unsigned int outlineIndices[] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

float crosshairVertices[] = {
    // Horizontal line (in NDC coordinates)
    -0.02f, 0.0f,
     0.02f, 0.0f,
    // Vertical line  
     0.0f, -0.02f,
     0.0f,  0.02f
};

unsigned int crosshairIndices[] = {
    0, 1,  // Horizontal line
    2, 3   // Vertical line
};




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
    //highlight voxel?


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

        if (game->castRayForVoxel(rayOrigin, rayDirection, hitVoxel, 50.0f, false)) {
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
    glm::ivec3 currentVoxel = glm::ivec3(std::floor(rayPos.x), std::floor(rayPos.y), std::floor(rayPos.z));  // Starting voxel (world coordinates)

    glm::ivec3 step = glm::ivec3(rayDirection.x > 0 ? 1 : -1,
                                 rayDirection.y > 0 ? 1 : -1,
                                 rayDirection.z > 0 ? 1 : -1);

    glm::vec3 tMax = (glm::vec3(currentVoxel) + glm::vec3(
        step.x > 0 ? 1.0f : 0.0f,
        step.y > 0 ? 1.0f : 0.0f,
        step.z > 0 ? 1.0f : 0.0f) - rayPos) / rayDirection;
    float distance = 0.0f;

    while (distance < maxDistance) {
        // Convert world coordinates to chunk-local coordinates
        glm::ivec3 chunkLocalVoxel = glm::ivec3(
            currentVoxel.x - static_cast<int>(chunk.position.x),
            currentVoxel.y - static_cast<int>(chunk.position.y),
            currentVoxel.z - static_cast<int>(chunk.position.z)
        );

        // Debug the current ray position (world coordinates)
        std::cout << "Raycasting at world voxel: (" << currentVoxel.x << ", " << currentVoxel.y << ", " << currentVoxel.z << ")" << std::endl;
        std::cout << "Chunk-local voxel: (" << chunkLocalVoxel.x << ", " << chunkLocalVoxel.y << ", " << chunkLocalVoxel.z << ")" << std::endl;

        // Check if the chunk-local coordinates are within bounds
        if (chunkLocalVoxel.x < 0 || chunkLocalVoxel.x >= chunk.sizeX ||
            chunkLocalVoxel.y < 0 || chunkLocalVoxel.y >= chunk.sizeY ||
            chunkLocalVoxel.z < 0 || chunkLocalVoxel.z >= chunk.sizeZ) {
            // Ray has left this chunk, break
            break;
        }

        // Check if the current voxel is solid using chunk-local coordinates
        if (chunk.isVoxelSolid(chunkLocalVoxel.x, chunkLocalVoxel.y, chunkLocalVoxel.z)) {
            hitVoxel = currentVoxel;  // Record the hit voxel (world coordinates)

            // Remove the voxel using chunk-local coordinates
            chunk.voxels[chunkLocalVoxel.x][chunkLocalVoxel.y][chunkLocalVoxel.z] = BlockType::Air;
            chunk.generateChunk();  // Regenerate the chunk
            chunk.setupMesh();  // Setup the mesh

            cout << "we hit a solid voxel" << endl;
            string coords = "(" + std::to_string(currentVoxel.x) + ", " + std::to_string(currentVoxel.y) + ", " + std::to_string(currentVoxel.z) + ")";
            string localCoords = "(" + std::to_string(chunkLocalVoxel.x) + ", " + std::to_string(chunkLocalVoxel.y) + ", " + std::to_string(chunkLocalVoxel.z) + ")";
            logger.log("INFO", "Hit solid voxel at world " + coords + " chunk-local " + localCoords);
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

        // This check is now done above after coordinate conversion
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


    ImGuiHandler::Shutdown();

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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);




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

    glGenVertexArrays(1, &cubeOutlineVAO);
    glGenBuffers(1, &cubeOutlineVBO);
    glGenBuffers(1, &cubeOutlineEBO);

    glBindVertexArray(cubeOutlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeOutlineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(outlineVertices), outlineVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeOutlineEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(outlineIndices), outlineIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); // clean


    glGenVertexArrays(1, &crosshairVAO);
    glGenBuffers(1, &crosshairVBO);
    glGenBuffers(1, &crosshairEBO);

    glBindVertexArray(crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, crosshairEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(crosshairIndices), crosshairIndices, GL_STATIC_DRAW);

    // Specify vertex attribute pointer for position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);





    logger.initialize(Log::INFO);
    ImGuiHandler::Initialize(window);

    // Initialize game objects
    float lastX = framebufferWidth / 2.0f;
    float lastY = framebufferHeight / 2.0f;

    // chunk = new Chunk(16,16,16, glm::vec3(0.0f, 0.0f, 0.0f) , this); ;
    camera = new Camera();
    shaderProgram = shaderLoader->loadShaders("VertShader.vertexshader", "FragShader.fragmentshader");
    outlineShader = shaderLoader->loadShaders("Outline.vert", "Outline.frag");
    crosshairShader = shaderLoader->loadShaders("CrosshairShader.vert", "CrosshairShader.frag");
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
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        isInteractingWithImGui = !isInteractingWithImGui; // Toggle interaction mode
        glfwSetInputMode(window, GLFW_CURSOR, isInteractingWithImGui ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }


    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


    if (!isInteractingWithImGui) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera->processInput(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera->processInput(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera->processInput(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera->processInput(RIGHT, deltaTime);
    }

}



void Game::Update(float deltaTime) {

    // glm::ivec3 hovered;
    // if (castRayForVoxel(camera->cameraPos, camera->cameraFront, hovered, 10.0f, true)) {
    //     selectedVoxel = hovered;
    // } else {
    //     selectedVoxel.reset();  // Clear highlight if no voxel hit
    // }

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
                    Chunk* newChunk = new Chunk(CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE, glm::vec3(x * CHUNK_SIZE, 0.0f, z * CHUNK_SIZE), this, shaderProgram, *textureManager);

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


Chunk* Game::getChunkAtWorldPosition(const glm::vec3& worldPos) {
    int chunkX = static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE));

    auto it = loadedChunks.find({chunkX, chunkZ});
    if (it != loadedChunks.end()) {
        return it->second;
    }
    return nullptr;
}

void Game::drawCrosshair() {
    // Use orthographic projection for 2D screen-space rendering
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

    glUseProgram(crosshairShader);
    glUniformMatrix4fv(glGetUniformLocation(crosshairShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Disable depth testing so crosshair is always visible
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(crosshairVAO);
    glDrawElements(GL_LINES, 4, GL_UNSIGNED_INT, 0);  // Draw 2 lines (4 vertices)
    glBindVertexArray(0);
    
    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
}


void Game::drawVoxelOutline(const glm::ivec3& voxelWorldPos, const glm::mat4& view, const glm::mat4& projection) {
    std::cout << "Drawing voxel outline at " << voxelWorldPos.x << ", " << voxelWorldPos.y << ", " << voxelWorldPos.z << std::endl;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(voxelWorldPos));
    glm::mat4 mvp = projection * view * model;

    glUseProgram(outlineShader);  // Use a dedicated wireframe shader
    glUniformMatrix4fv(glGetUniformLocation(outlineShader, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(cubeOutlineVAO);  // A cube VAO that just draws edges
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);  // 12 edges = 24 indices
}


bool Game::castRayForVoxel(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::ivec3& hitVoxel, float maxDistance, bool forHighlight) {
    glm::vec3 rayPos = rayOrigin;
    glm::vec3 stepSize = glm::vec3(1.0f) / glm::abs(rayDirection);
    glm::ivec3 currentVoxel = glm::ivec3(std::floor(rayPos.x), std::floor(rayPos.y), std::floor(rayPos.z));

    glm::ivec3 step = glm::ivec3(rayDirection.x > 0 ? 1 : -1,
                                 rayDirection.y > 0 ? 1 : -1,
                                 rayDirection.z > 0 ? 1 : -1);

    glm::vec3 tMax = (glm::vec3(currentVoxel) + glm::vec3(
        step.x > 0 ? 1.0f : 0.0f,
        step.y > 0 ? 1.0f : 0.0f,
        step.z > 0 ? 1.0f : 0.0f) - rayPos) / rayDirection;

    float distance = 0.0f;
    Chunk* lastChunk = nullptr;

    while (distance < maxDistance) {
        // Get the chunk at current world position
        glm::vec3 worldPos = glm::vec3(currentVoxel.x, currentVoxel.y, currentVoxel.z);
        Chunk* currentChunk = getChunkAtWorldPosition(worldPos);

        if (currentChunk != nullptr) {
            // Log chunk change
            if (currentChunk != lastChunk) {
                int chunkX = static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE));
                int chunkZ = static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE));
                string chunkName = "Chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkZ);
                logger.log("INFO", "Ray entered " + chunkName);
                lastChunk = currentChunk;
            }

            // Convert world coordinates to chunk-local coordinates
            glm::ivec3 chunkLocalVoxel = glm::ivec3(
                currentVoxel.x - static_cast<int>(currentChunk->position.x),
                currentVoxel.y - static_cast<int>(currentChunk->position.y),
                currentVoxel.z - static_cast<int>(currentChunk->position.z)
            );

            // Check bounds within chunk
            if (chunkLocalVoxel.x >= 0 && chunkLocalVoxel.x < currentChunk->sizeX &&
                chunkLocalVoxel.y >= 0 && chunkLocalVoxel.y < currentChunk->sizeY &&
                chunkLocalVoxel.z >= 0 && chunkLocalVoxel.z < currentChunk->sizeZ) {

                // Check if the current voxel is solid
                if (currentChunk->isVoxelSolid(chunkLocalVoxel.x, chunkLocalVoxel.y, chunkLocalVoxel.z)) {
                    hitVoxel = currentVoxel;  // Record the hit voxel (world coordinates)

                    PRINT_BLOCK_TYPE(currentChunk->voxels[chunkLocalVoxel.x][chunkLocalVoxel.y][chunkLocalVoxel.z])
                    // Remove the voxel using chunk-local coordinates
                    if (!forHighlight){
                       
                    
                        currentChunk->voxels[chunkLocalVoxel.x][chunkLocalVoxel.y][chunkLocalVoxel.z] = BlockType::Air;
                        currentChunk->generateChunk();
                        currentChunk->setupMesh();

                        string coords = "(" + std::to_string(currentVoxel.x) + ", " + std::to_string(currentVoxel.y) + ", " + std::to_string(currentVoxel.z) + ")";
                        string localCoords = "(" + std::to_string(chunkLocalVoxel.x) + ", " + std::to_string(chunkLocalVoxel.y) + ", " + std::to_string(chunkLocalVoxel.z) + ")";
                        logger.log("INFO", "Hit solid voxel at world " + coords + " chunk-local " + localCoords);

                        return true;
                    }

                    else{
                        string coords = "(" + std::to_string(currentVoxel.x) + ", " + std::to_string(currentVoxel.y) + ", " + std::to_string(currentVoxel.z) + ")";
                        string localCoords = "(" + std::to_string(chunkLocalVoxel.x) + ", " + std::to_string(chunkLocalVoxel.y) + ", " + std::to_string(chunkLocalVoxel.z) + ")";
                        logger.log("INFO", "Hit solid voxel at world " + coords + " chunk-local " + localCoords);
                        return true;
                    }
                }
            }
        }

        // Move to the next voxel boundary
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

        // Break if we go too far underground or too high
        if (currentVoxel.y < 0 || currentVoxel.y > 64) {
            break;
        }
    }

    logger.log("INFO", "Ray hit nothing");
    return false;
}
void Game::Render() {
    // Enable wireframe mode for debugging (if needed)
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glm::ivec3 hoveredVoxel;
    bool hasHit = castRayForVoxel(camera->cameraPos, camera->cameraFront, hoveredVoxel, 50, /*highlight*/ true);
    std::optional<glm::ivec3> selectedVoxel;
    
    // Set selectedVoxel based on raycast result
    if (hasHit) {
        selectedVoxel = hoveredVoxel;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Set the sky color to light sky blue
    glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
    logger.checkOpenGLError("After clear");

    // Validate shader program before use
    if (shaderProgram == 0) {
        logger.setLevel(Log::ERROR, "Invalid shader program in render");
        return;
    }

    // Set up lighting and ambient colors
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientColor = glm::vec3(0.53f, 0.81f, 0.98f);

    glUseProgram(shaderProgram);
    logger.checkOpenGLError("After shader program");

    // Pass light information to the shader with validation
    GLuint lightDirLoc = glGetUniformLocation(shaderProgram, "lightDir");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint ambientColorLoc = glGetUniformLocation(shaderProgram, "ambientColor");

    if (lightDirLoc != -1) {
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    }
    if (lightColorLoc != -1) {
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    }
    if (ambientColorLoc != -1) {
        glUniform3fv(ambientColorLoc, 1, glm::value_ptr(ambientColor));
    }
    logger.checkOpenGLError("After light uniforms");

    // Validate and bind texture
    if (this->textureID == 0) {
        logger.setLevel(Log::WARNING, "Invalid texture ID in render");
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->textureID);
        GLint textureUniformLoc = glGetUniformLocation(shaderProgram, "blockTexture");
        if (textureUniformLoc != -1) {
            glUniform1i(textureUniformLoc, 0);
        }
        logger.checkOpenGLError("After texture binding");
    }

    // Update matrices
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    // Render all chunks with validation
    logger.setLevel(Log::DEBUG, "Rendering " + std::to_string(loadedChunks.size()) + " chunks");
    for (const auto& chunkPair : loadedChunks) {
        if (chunkPair.second != nullptr) {
            chunkPair.second->render(shaderProgram, view, projection);
            logger.checkOpenGLError("After chunk render");
        }
    }

    if (selectedVoxel.has_value()) {
        drawVoxelOutline(selectedVoxel.value(), view, projection);
    }

    // Save OpenGL state before ImGui
    OpenGLState state;
    SaveOpenGLState(state);
    logger.checkOpenGLError("After saving OpenGL state");

    // Ensure we're in the right state for ImGui
    glUseProgram(0);  // Clear shader program
    glBindTexture(GL_TEXTURE_2D, 0);  // Clear texture binding
    glBindVertexArray(0);  // Clear VAO binding

    // Render ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawCrosshair();

    ImGui::Begin("Log");
    logger.displayLog();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    logger.checkOpenGLError("After ImGui render");

    // Restore OpenGL state after ImGui
    RestoreOpenGLState(state);
    logger.checkOpenGLError("After restoring OpenGL state");

    // Swap buffers to display the rendered frame
    glfwSwapBuffers(window);
}




void Game::Run() {
    Init();

    logger.setLevel(Log::INFO, "Game running...");

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input first to ensure responsive controls
        ProcessInput(deltaTime);

        // Update game state
        Update(deltaTime);

        // Render the frame
        Render();

        // Poll for events
        glfwPollEvents();
    }

    logger.setLevel(Log::INFO, "Game shutting down...");
}
