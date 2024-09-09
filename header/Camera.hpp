#pragma once
#include <glm/glm.hpp>
#include "ShaderLoader.hpp"



enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
    public:
        Camera();
        ~Camera();
        void update();
        void processInput(Camera_Movement direction, float deltaTime);
        glm::mat4 getViewMatrix();
        void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
        void updateCameraVectors();
        void processMouseScroll(float yoffset);
        glm::vec3 cameraPos; // current position of the camera
        glm::vec3 cameraTarget; // where the camera is looking at
        glm::vec3 cameraDirection; // direction from the camera to the target
        glm::vec3 up; // up vector
        glm::vec3 cameraRight; // right vector
        glm::vec3 cameraUp; // up vector
        glm::vec3 cameraFront; // front vector
        GLuint shaderProgram;
        ShaderLoader *shaderLoader;
        void render(GLuint shaderProgram, float rayLength);
        GLuint rayVAO, rayVBO;  // Add this in Camera class or Game class
        void initRayRendering();
        int width, height;


    private:
        
        float deltaTime = 0.0f; // time between current frame and last frame
        float lastFrame = 0.0f; // time of last frame
        float Yaw;
        float Pitch;
        // camera options
        float MovementSpeed;
        float MouseSensitivity;
        float Zoom;
        
};