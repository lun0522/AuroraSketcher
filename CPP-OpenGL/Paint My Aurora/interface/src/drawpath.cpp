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
#include "crspline.hpp"
#include "drawpath.hpp"

using std::string;
using std::runtime_error;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

const float EARTH_RADIUS = 6378.1f;
const float AURORA_HEIGHT = 100.0f;
const float AURORA_RELA_HEIGHT = (EARTH_RADIUS + AURORA_HEIGHT) / EARTH_RADIUS;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const float INERTIAL_COEFF = 1.5f;

vec3 cameraPos(0.0f, 0.0f, 30.0f);
Camera camera(cameraPos);
vec2 originalSize, currentSize, retinaRatio;
vec2 mousePos, posOffset;
bool isNight = true;
bool isClicking = false;
bool enableRotation = true;
bool shouldUpdateCamera = false;

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    currentSize = vec2(width, height);
    // keep earth in center after resizing window
    glViewport((currentSize.x - originalSize.x) * 0.5f,
               (currentSize.y - originalSize.y) * 0.5f,
               originalSize.x, originalSize.y);
    posOffset = (originalSize - currentSize) * 0.5f;
}

void mouseClickCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) isClicking = action == GLFW_PRESS;
}

void mouseScrollCallback(GLFWwindow *window, double xPos, double yPos) {
    camera.processMouseScroll(yPos, 15.0f, 45.0f);
    shouldUpdateCamera = true;
}

void DrawPath::updateMousePos() {
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    mousePos = vec2(xPos, yPos);
}

void DrawPath::processKeyboardInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) isNight = false;
        else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) isNight = true;
        
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) enableRotation = false;
        else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) enableRotation = true;
    }
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
    // store shared matrices
    
    GLuint uboMatrices;
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_DYNAMIC_DRAW); // no data yet
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrices); // or use glBindBufferRange for flexibility
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    
    // ------------------------------------
    // earth
    
    Model earth(directory + "texture/earth/earth.obj");
    GLuint earthDayTex = Loader::loadTexture(directory + "texture/earth/earth_day.jpg", false);
    GLuint earthNightTex = Loader::loadTexture(directory + "texture/earth/earth_night.jpg", false);
    Shader earthShader(directory + "interface/shaders/earth.vs",
                       directory + "interface/shaders/earth.fs");
    
    auto initialRotation = [] (mat4& model) {
        // north pole will be in the center of the frame
        model = glm::rotate(model, glm::radians( 90.0f), vec3( 1.0f,  0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), vec3( 0.0f, -1.0f, 0.0f));
    };
    
    earthShader.use();
    
    mat4 earthModel(1.0f);
    earthModel = glm::scale(earthModel, vec3(10.0f)); // scaling at last is okay for sphere
    initialRotation(earthModel);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, earthDayTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, earthNightTex);
    
    // earth model is shared with the spline
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), glm::value_ptr(earthModel));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    
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
    initialRotation(universeModel);
    universeShader.setMat4("model", universeModel);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, universeTex);
    universeShader.setInt("cubemap0", 2);
    
    
    // ------------------------------------
    // aurora path
    
    std::vector<vec3> controlPoints;
    float latitude = glm::radians(66.7f);
    float sinLat = sin(latitude), cosLat = cos(latitude);
    for (float angle = 0.0f; angle < 360.0f; angle += 45.0f)
        controlPoints.push_back(vec3(cos(glm::radians(angle)) * cosLat, sinLat, sin(glm::radians(angle)) * cosLat));
    
    CRSpline spline(controlPoints, AURORA_RELA_HEIGHT);
    
    Shader pointShader(directory + "interface/shaders/spline.vs",
                       directory + "interface/shaders/spline.fs",
                       directory + "interface/shaders/spline.gs");
    pointShader.use();
    pointShader.setFloat("ratio", currentSize.x / currentSize.y);
    
    Shader curveShader(directory + "interface/shaders/spline.vs",
                       directory + "interface/shaders/spline.fs");
    
    
    // ------------------------------------
    // render
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    int frameCount = 0;
    float rotAngle = 0.0f, scrollStartTime = 0.0f, lastTime = glfwGetTime();
    mat4 worldToNDC, ndcToWorld, ndcToEarth, worldToEarth = glm::inverse(earthModel);
    vec3 lastIntersect, rotAxis, cameraEarth = worldToEarth * vec4(cameraPos, 1.0);;
    bool shouldRotate = false, shouldScroll = false;
    
    auto updateCamera = [&] () {
        worldToNDC = camera.getProjectionMatrix() * camera.getViewMatrix();
        ndcToWorld = glm::inverse(worldToNDC);
        ndcToEarth = worldToEarth * ndcToWorld;
        
        // worldToNDC i.e. viewProjection is shared everywhere
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), glm::value_ptr(worldToNDC));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    };
    updateCamera(); // initialize matrices declared above
    
    auto rotateScene = [&] (const float angle, const vec3& axis) {
        earthModel = glm::rotate(earthModel, angle, axis);
        worldToEarth = glm::inverse(earthModel);
        ndcToEarth = worldToEarth * ndcToWorld;
        cameraEarth = worldToEarth * vec4(cameraPos, 1.0);
        
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), glm::value_ptr(earthModel));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        
        universeModel = glm::rotate(universeModel, angle, axis);
        universeShader.use();
        universeShader.setMat4("model", universeModel);
    };
    
    auto scrollOnCondition = [&] () {
        if (!shouldScroll) {
            // start scrolling if rotated at the last frame, and:
            // (1) the click point is out of the earth at this frame, or
            // (2) the mouse key is released in this frame
            if (shouldRotate) {
                shouldScroll = true;
                scrollStartTime = glfwGetTime();
            }
            shouldRotate = false;
        }
    };
    
    auto getIntersection = [&] (vec3& pos, vec3& norm) -> bool {
        vec2 clickScreen = (mousePos * retinaRatio + posOffset) / originalSize; // [0.0, 1.0]
        clickScreen = clickScreen * 2.0f - 1.0f; // [-1.0, 1.0]
        clickScreen.y *= -1.0f; // flip y coordinate
        vec4 clickEarth = ndcToEarth * vec4(clickScreen, 1.0f, 1.0f);
        clickEarth /= clickEarth.w; // important! perpective matrix may not be normalized
        return glm::intersectRaySphere(cameraEarth, glm::normalize(vec3(clickEarth) - cameraEarth),
                                       vec3(0.0f), 1.0f, pos, norm);
    };
    
    while (!glfwWindowShouldClose(window)) {
        processKeyboardInput();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (shouldUpdateCamera) {
            shouldUpdateCamera = false;
            updateCamera();
        }
        
        if (!isClicking) {
            scrollOnCondition();
        } else {
            updateMousePos();
            vec3 intersectPos, intersectNorm;
            bool didIntersect = getIntersection(intersectPos, intersectNorm);
            
            if (!enableRotation) {
                spline.processMouseClick(intersectPos);
                scrollOnCondition();
            } else {
                if (!didIntersect) {
                    scrollOnCondition();
                } else {
                    if (shouldRotate) {
                        float angle = glm::angle(lastIntersect, intersectPos);
                        if (angle > 3E-3) {
                            rotAngle = angle;
                            rotAxis = glm::cross(lastIntersect, intersectPos);
                            rotateScene(rotAngle, rotAxis);
                        }
                    } else {
                        // update lastIntersect only at the first intersection
                        // the intersection point on the earth should not change
                        // until the click stops
                        lastIntersect = intersectPos;
                        shouldRotate = true;
                        rotAngle = 0.0f; // important when a click happens during inertial scrolling
                    }
                    shouldScroll = false; // stop inertial scrolling
                }
            }
        }
        
        if (shouldScroll) {
            float elapsedTime = glfwGetTime() - scrollStartTime;
            if (rotAngle == 0.0f || elapsedTime > INERTIAL_COEFF) {
                shouldScroll = false;
            } else {
                elapsedTime /= INERTIAL_COEFF;
                float frac = (1.0f - elapsedTime * elapsedTime);
                rotateScene(rotAngle * frac, rotAxis);
            }
        }
        
        earthShader.use();
        earthShader.setInt("texture0", isNight? 1 : 0);
        earth.draw(earthShader);
        
        pointShader.use();
        spline.drawControlPoints();
        
        curveShader.use();
        spline.drawSplineCurve();
        
        glDepthFunc(GL_LEQUAL);
        universe.draw(universeShader);
        glDepthFunc(GL_LESS);
        
        glfwSwapBuffers(window); // use color buffer to draw
        glfwPollEvents(); // check events (keyboard, mouse, ...)
        
        ++frameCount;
        float currentTime = glfwGetTime();
        if (currentTime - lastTime > 1.0) {
            std::cout <<  "FPS: " << std::to_string(frameCount) << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }
}
