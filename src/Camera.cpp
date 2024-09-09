#include "Camera.hpp"
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>



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
    width = 800;
    height = 600;
    shaderProgram = shaderLoader->loadShaders("VertShaderRay.vertexshader", "FragShaderRay.fragmentshader");
    initRayRendering();
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

void checkGLError(const std::string& location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << location << ": " << err << std::endl;
    }
}

void Camera::initRayRendering() {
    // Generate VAO and VBO for the ray
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);
    checkGLError("initRayRendering: After VAO and VBO generation");

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    checkGLError("initRayRendering: After binding VAO and VBO");

    // We will store two vertices (start and end of the line)
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    checkGLError("initRayRendering: After setting buffer data");

    // Set up vertex attributes for position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    checkGLError("initRayRendering: After setting vertex attribute pointer");

    glEnableVertexAttribArray(0);
    checkGLError("initRayRendering: After enabling vertex attribute");

    glBindVertexArray(0);  // Unbind the VAO
    checkGLError("initRayRendering: After unbinding VAO");
}

void Camera::render(GLuint shaderProgram, float rayLength) {
    std::cout << "Rendering ray" << std::endl;
    glUseProgram(shaderProgram);
    checkGLError("render: After glUseProgram");

    // Calculate the start (camera position) and end of the ray (cameraFront * length)
    glm::vec3 rayEnd = cameraPos + cameraFront * 100.0f;  // Keep the ray length minimal for testing
    cout << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << endl;
    cout << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << endl;
    cout << rayEnd.x << ", " << rayEnd.y << ", " << rayEnd.z << endl;

    // Define the ray vertices
    glm::vec3 vertices[2] = { cameraPos, rayEnd };  // Simple line in front of the camera

    // glm::vec3 vertices[2] = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -5.0f) };  // Simple line in front of the camera
    
    cout << "Ray Start: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << endl;
    cout << "Ray End: " << rayEnd.x << ", " << rayEnd.y << ", " << rayEnd.z << endl;

    // Update the VBO with the new ray vertices
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    checkGLError("render: After binding VBO");
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    checkGLError("render: After updating VBO data");

    // Pass the view and projection matrices to the shader
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(Zoom), (float)width / (float)height, 0.1f, 100.0f);
    
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    checkGLError("render: After getting view uniform location");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    checkGLError("render: After getting projection uniform location");
    GLuint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    checkGLError("render: After getting objectColor uniform location");

    if (viewLoc != -1 && projLoc != -1 && colorLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        checkGLError("render: After setting view matrix uniform");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        checkGLError("render: After setting projection matrix uniform");

        // Set the ray color (white for clarity)
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        checkGLError("render: After setting color uniform");
    } else {
        std::cerr << "Error: Uniform location not found." << std::endl;
    }

    // Set a visible line width
    // glLineWidth(2.0f);
    checkGLError("render: After setting line width");

    // Disable depth test to ensure ray is visible
    glDisable(GL_DEPTH_TEST);
    checkGLError("render: After disabling depth test");
    
    // Bind the VAO and draw the ray as a line
    glBindVertexArray(rayVAO);
    checkGLError("render: After binding VAO");
    glDrawArrays(GL_LINES, 0, 2);
    checkGLError("render: After drawing arrays");
    glBindVertexArray(0);
    checkGLError("render: After unbinding VAO");

    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
    checkGLError("render: After enabling depth test");
}
