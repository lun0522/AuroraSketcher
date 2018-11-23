//
//  window.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/15/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include "window.hpp"

#include <stdexcept>

#include "drawpath.hpp"

using namespace std;
using namespace glm;

static DrawPath *pathEditor;
static Window *currentWindow;

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    currentWindow->setSize(width, height);
}

void mouseClickCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT)
        pathEditor->didClickMouse(button == GLFW_MOUSE_BUTTON_LEFT, action == GLFW_PRESS);
}

void mouseScrollCallback(GLFWwindow *window, double xPos, double yPos) {
    pathEditor->didScrollMouse(yPos);
}

void mouseMoveCallback(GLFWwindow *window, double xPos, double yPos) {
    pathEditor->didMoveMouse(vec2(xPos, yPos));
}

const vec4& Window::getViewPort() const {
    return viewPort;
}

const vec2& Window::getOriginalSize() const {
    return originalSize;
}

const vec2& Window::getClickNDC() const {
    return clickNDC;
}

void Window::setSize(int width, int height) {
    currentSize = vec2(width, height);
    // keep earth in center after resizing window
    posOffset = (originalSize - currentSize) * 0.5f;
    setViewPort(vec4((currentSize - originalSize) * 0.5f, originalSize));
}

void Window::setViewPort(const vec4& value) {
    viewPort = value;
    glViewport(viewPort.x, viewPort.y, viewPort.z, viewPort.w); // specify render area
}

void Window::updateMousePos() {
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    vec2 clickScreen = (vec2(xPos, yPos) * retinaRatio + posOffset) / originalSize; // [0.0, 1.0]
    clickNDC = clickScreen * 2.0f - 1.0f; // [-1.0, 1.0]
    clickNDC.y *= -1.0f; // flip y coordinate
}

void Window::setCaptureCursor(const bool value) const {
    glfwSetInputMode(window, GLFW_CURSOR, value ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Window::processKeyboardInput() const {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

Window::Window(DrawPath *drawPath, int width, int height) {
    pathEditor = drawPath;
    currentWindow = this;
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // ------------------------------------
    // window
    
    window = glfwCreateWindow(width, height, "Draw My Aurora", NULL, NULL);
    if (!window) throw runtime_error("Failed to create window");
    
    glfwMakeContextCurrent(window);
    // called when window is resized by the user
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseClickCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    
    // ------------------------------------
    // GLAD (function pointer loader)
    // do this after context is created, and before calling any OpenGL function!
    
    if (!gladLoadGL()) throw runtime_error("Failed to init GLAD");
    
    // screen size is different from the input width and height on retina screen
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    originalSize = currentSize = vec2(screenWidth, screenHeight);
    retinaRatio = originalSize / vec2(width, height);
    setViewPort(vec4(0, 0, screenWidth, screenHeight));
}

void Window::renderFrame() const {
    glfwSwapBuffers(window); // use color buffer to draw
    glfwPollEvents(); // check events (keyboard, mouse, ...)
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window);
}
