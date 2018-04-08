//
//  drawpath.cpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <iostream>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "loader.hpp"
#include "model.hpp"
#include "drawpath.hpp"

using std::string;
using std::runtime_error;
using glm::ivec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

vec3 cameraPos(0.0f, 0.0f, 30.0f);
Camera camera(cameraPos);
vec2 originalSize, currentSize, retinaRatio;
vec2 mousePos, posOffset;
bool isClicking = false;
bool shouldUpdateMatrices = true;

void updateMousePos(GLFWwindow *window) {
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    mousePos = vec2(xPos, yPos);
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    currentSize = vec2(width, height);
    // keep earth in center after resizing window
    glViewport((currentSize.x - originalSize.x) * 0.5f,
               (currentSize.y - originalSize.y) * 0.5f,
               originalSize.x, originalSize.y);
    posOffset = (originalSize - currentSize) * 0.5f;
}

void mouseClickCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) isClicking = true;
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) isClicking = false;
}

void mouseScrollCallback(GLFWwindow *window, double xPos, double yPos) {
    camera.processMouseScroll(yPos, 10.0f, 45.0f);
    shouldUpdateMatrices = true;
}

void DrawPath::processKeyboardInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

DrawPath::DrawPath(const char *directory):
directory(directory) {
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
    glfwSetMouseButtonCallback(window, mouseClickCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    
    // ------------------------------------
    // GLAD (function pointer loader)
    // do this after context is created, and before calling any OpenGL function!
    
    if (!gladLoadGL()) throw runtime_error("Failed to init GLAD");
    
    // screen size is different from the input width and height on retina screen
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height); // specify render area
    camera.setScreenSize(width, height);
    currentSize = vec2(width, height);
    originalSize = currentSize;
    retinaRatio = originalSize / vec2(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void DrawPath::mainLoop() {
    // ------------------------------------
    // earth
    
    Model earth(directory + "texture/earth/earth.obj");
    GLuint earthTex = Loader::loadTexture(directory + "texture/earth/earthmap.jpg", false);
    Shader earthShader(directory + "interface/shaders/earth.vs",
                       directory + "interface/shaders/earth.fs");
    
    earthShader.use();
    
    mat4 earthModel(1.0f);
    earthModel = glm::scale(earthModel, vec3(10.0f)); // scaling at last is okay for sphere
    earthModel = glm::rotate(earthModel, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    earthModel = glm::rotate(earthModel, glm::radians(-100.0f), vec3(0.0f, -1.0f, 0.0f));
    earthShader.setMat4("model", earthModel);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthTex);
    earthShader.setInt("texture0", 0);
    
    
    // ------------------------------------
    // universe
    
    Model universe(directory + "texture/universe/skybox.obj");
    std::vector<string> boxfaces {
        "PositiveX.jpg",
        "NegativeX.jpg",
        "PositiveY.jpg",
        "NegativeY.jpg",
        "PositiveZ.jpg",
        "NegativeZ.jpg",
    };
    GLuint universeTex = Loader::loadCubemap(directory + "texture/universe", boxfaces, false);
    Shader universeShader(directory + "interface/shaders/universe.vs",
                          directory + "interface/shaders/universe.fs");
    
    universeShader.use();
    
    mat4 universeModel(1.0f);
    universeModel = glm::translate(universeModel, camera.getPosition());
    universeModel = glm::rotate(universeModel, glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    universeModel = glm::rotate(universeModel, glm::radians(-100.0f), vec3(0.0f, -1.0f, 0.0f));
    universeShader.setMat4("model", universeModel);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, universeTex);
    universeShader.setInt("cubemap0", 1);
    
    
    // ------------------------------------
    // render
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    int frameCount = 0;
    double lastTime = glfwGetTime();
    mat4 worldToNDC, ndcToEarth, worldToEarth = glm::inverse(earthModel);
    vec3 cameraEarth, lastIntersect;
    bool didIntersect = false;
    
    while (!glfwWindowShouldClose(window)) {
        processKeyboardInput();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (shouldUpdateMatrices) {
            shouldUpdateMatrices = false;
            worldToNDC = camera.getProjectionMatrix() * camera.getViewMatrix();
            ndcToEarth = worldToEarth * glm::inverse(worldToNDC);
            cameraEarth = worldToEarth * vec4(cameraPos, 1.0);
            
            earthShader.use();
            earthShader.setMat4("viewProjection", worldToNDC);
            universeShader.use();
            universeShader.setMat4("viewProjection", worldToNDC);
        }
        
        updateMousePos(window);
        if (isClicking && frameCount > 0) {
            vec2 clickScreen = (mousePos * retinaRatio + posOffset) / originalSize; // [0.0, 1.0]
            clickScreen = clickScreen * 2.0f - 1.0f; // [-1.0, 1.0]
            clickScreen.y *= -1.0f; // flip y coordinate
            vec4 clickEarth = ndcToEarth * vec4(clickScreen, 1.0f, 1.0f);
            clickEarth /= clickEarth.w; // important! perpective matrix may not be normalized
            
            vec3 intersectPos, intersectNorm;
            if (glm::intersectRaySphere(cameraEarth, glm::normalize(vec3(clickEarth) - cameraEarth),
                                        vec3(0.0f), 1.0f, intersectPos, intersectNorm)) {
                if (didIntersect) {
                    float angle = glm::angle(lastIntersect, intersectPos);
                    if (angle > 1E-2) {
                        vec3 axis = glm::cross(lastIntersect, intersectPos);
                        earthModel = glm::rotate(earthModel, angle, axis);
                        earthShader.use();
                        earthShader.setMat4("model", earthModel);
                    }
                }
                lastIntersect = intersectPos;
                didIntersect = true;
            } else {
                didIntersect = false;
            }
        } else {
            didIntersect = false;
        }
        
        earth.draw(earthShader);
        
        glDepthFunc(GL_LEQUAL);
        universe.draw(universeShader);
        glDepthFunc(GL_LESS);
        
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
