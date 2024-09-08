#include "Camera.hpp"
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;



const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

Camera::Camera(){
    cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraDirection = glm::normalize(cameraPos - cameraTarget);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    cameraUp = glm::cross(cameraDirection, cameraRight);
    glm::mat4 view;
    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), 
  		   glm::vec3(0.0f, 0.0f, 0.0f), 
  		   glm::vec3(0.0f, 1.0f, 0.0f));
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    MovementSpeed = SPEED;
    MouseSensitivity = SENSITIVITY;
    Zoom = ZOOM;
    Yaw = YAW;
    Pitch = PITCH;

}

Camera::~Camera() {
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch){
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f) {
            Pitch = 89.0f;
        }
        if (Pitch < -89.0f) {
            Pitch = -89.0f;
        }
    }

    updateCameraVectors();

}

void Camera::processMouseScroll(float yoffset){
    Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
}

void Camera::updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        cameraFront = glm::normalize(front);
        // also re-calculate the Right and Up vector
        cameraRight = (glm::cross(cameraFront, up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
    }

void Camera::processInput(Camera_Movement direction, float deltaTime){
    float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            cameraPos += cameraFront * velocity;
        if (direction == BACKWARD)
            cameraPos -= cameraFront * velocity;
        if (direction == LEFT)
            cameraPos -= cameraRight * velocity;
        if (direction == RIGHT)
            cameraPos += cameraRight * velocity;
}

glm::mat4 Camera::getViewMatrix(){
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void Camera::update(){
    
   
}