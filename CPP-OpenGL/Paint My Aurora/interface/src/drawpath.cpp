//
//  drawpath.cpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <iostream>
#include <string>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "loader.hpp"
#include "model.hpp"
#include "drawpath.hpp"

using std::string;
using std::runtime_error;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

typedef struct ScreenSize {
    int width;
    int height;
} ScreenSize;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

float lastFrame = 0.0f;
float explosion = 0.0f;
Camera camera(vec3(0.0f, 0.0f, 30.0f));
ScreenSize originalSize{0, 0};
ScreenSize currentSize{0, 0};

void updateScreenSize(GLFWwindow *window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    currentSize.width = width;
    currentSize.height = height;
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    currentSize.width = width;
    currentSize.height = height;
    glViewport(0, 0, width, height);
}

void mouseMoveCallback(GLFWwindow *window, double xPos, double yPos) {
    camera.processMouseMove(xPos, yPos);
}

void mouseScrollCallback(GLFWwindow *window, double xPos, double yPos) {
    camera.processMouseScroll(yPos);
}

void DrawPath::processKeyboardInput() {
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return;
    }
    
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    camera.processKeyboardInput(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    camera.processKeyboardInput(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    camera.processKeyboardInput(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    camera.processKeyboardInput(RIGHT, deltaTime);
}

DrawPath::DrawPath() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // ------------------------------------
    // window
    
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Paint My Aurora", NULL, NULL);
    if (window == NULL) throw runtime_error("Failed to create window");
    
    glfwMakeContextCurrent(window);
    // called when window is resized by the user
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    
    // ------------------------------------
    // GLAD (function pointer loader)
    // do this after context is created, and before calling any OpenGL function!
    
    if (!gladLoadGL()) throw runtime_error("Failed to init GLAD");
    
    // screen size is different from the input width and height on retina screen
    updateScreenSize(window);
    originalSize = currentSize;
    glViewport(0, 0, currentSize.width, currentSize.height); // specify render area
    camera.setScreenSize(currentSize.width, currentSize.height);
}

void DrawPath::mainLoop() {
    string path = "/Users/lun/Desktop/Code/Aurora/CPP-OpenGL/Paint My Aurora/";
    Model earth(path + "texture/earth/earth.obj");
    GLuint earthTex = Loader::loadTexture(path + "texture/earth/earthmap.jpg", false);
    
    Shader earthShader(path + "interface/shaders/earth.vs",
                       path + "interface/shaders/earth.fs");
    mat4 earthModel(1.0f);
    earthModel = glm::rotate(earthModel, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    earthModel = glm::rotate(earthModel, glm::radians(-100.0f), vec3(0.0f, -1.0f, 0.0f));
    earthModel = glm::scale(earthModel, vec3(10.0f));
    earthShader.use();
    earthShader.setMat4("model", earthModel);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthTex);
    earthShader.setInt("texture0", 0);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    int frameCount = 0;
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        processKeyboardInput();
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mat4 view = camera.getViewMatrix();
        mat4 projection = camera.getProjectionMatrix();
        
        earthShader.use();
        earthShader.setMat4("view", view);
        earthShader.setMat4("projection", projection);
        earth.draw(earthShader);
        
        glfwSwapBuffers(window); // use color buffer to draw
        glfwPollEvents(); // check events (keyboard, mouse, ...)
        
        ++frameCount;
        double currentTime = glfwGetTime();
        if (currentTime - lastTime > 1.0) {
            std::cout <<  "FPS: " << std::to_string(frameCount) << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }
}
