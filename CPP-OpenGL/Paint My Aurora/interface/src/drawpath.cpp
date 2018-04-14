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
#include "button.hpp"
#include "crspline.hpp"
#include "drawpath.hpp"

using std::string;
using std::runtime_error;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

static const float EARTH_RADIUS = 6378.1f;
static const float AURORA_HEIGHT = 100.0f;
static const float AURORA_RELA_HEIGHT = (EARTH_RADIUS + AURORA_HEIGHT) / EARTH_RADIUS;
static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 600;
static const float CTRL_POINT_SIDE_LENGTH = 20.0f;
static const float CLICK_CTRL_POINT_TOLERANCE = 10.0f;
static const float INERTIAL_COEFF = 1.5f;

vec3 cameraPos(0.0f, 0.0f, 30.0f);
Camera camera(cameraPos);
vec2 originalSize, currentSize, retinaRatio;
vec2 mousePos, posOffset;
bool isNight = true;
bool isClicking = false;
bool didClickRight = false;
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
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        isClicking = action == GLFW_PRESS;
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) isClicking = didClickRight = true;
        // do nothing when the right mouse key is released
        // these two boolean states will be reset after intersection detection
    }
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
    // buttons
    
    Button lockButton("Lock Earth",
                      directory + "texture/button/quad.obj",
                      directory + "texture/button/rect_rounded.jpg",
                      directory + "interface/shaders/button.vs",
                      directory + "interface/shaders/button.fs",
                      vec2(0.1f, 0.06f), vec2(0.16f, 0.08f),
                      vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 1.0f) * 0.3f);
    
    std::vector<Button> buttons = {
        lockButton,
    };
    
    
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
    vec2 sideLengthNDC = vec2(CTRL_POINT_SIDE_LENGTH) / originalSize * 2.0f;
    pointShader.setVec2("sideLength", sideLengthNDC);
    sideLengthNDC *= (CTRL_POINT_SIDE_LENGTH + CLICK_CTRL_POINT_TOLERANCE) / CTRL_POINT_SIDE_LENGTH;
    
    Shader curveShader(directory + "interface/shaders/spline.vs",
                       directory + "interface/shaders/spline.fs");
    
    
    // ------------------------------------
    // render
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    int frameCount = 0;
    float rotAngle = 0.0f, scrollStartTime = 0.0f, lastTime = glfwGetTime();
    mat4 worldToNDC, ndcToWorld, ndcToEarth, worldToEarth = glm::inverse(earthModel);
    vec3 lastIntersect, rotAxis, cameraEarth = worldToEarth * vec4(cameraPos, 1.0);
    vec2 clickNDC;
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
    
    auto renderScene = [&] () {
        earthShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, isNight ? earthNightTex : earthDayTex);
        earthShader.setInt("texture0", 0);
        earth.draw(earthShader);
        
        pointShader.use();
        spline.drawControlPoints();
        
        curveShader.use();
        spline.drawSplineCurve();
        
        std::for_each(buttons.begin(), buttons.end(),
                      [] (Button& button) { button.draw(); });
        
        glDepthFunc(GL_LEQUAL);
        universeShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, universeTex);
        universeShader.setInt("cubemap", 0);
        universe.draw(universeShader);
        glDepthFunc(GL_LESS);
    };
    
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
    
    auto getIntersection = [&] (vec3& position, vec3& normal, const float radius = 1.0f) -> bool {
        vec2 clickScreen = (mousePos * retinaRatio + posOffset) / originalSize; // [0.0, 1.0]
        clickNDC = clickScreen * 2.0f - 1.0f; // [-1.0, 1.0]
        clickNDC.y *= -1.0f; // flip y coordinate
        vec4 clickEarth = ndcToEarth * vec4(clickNDC, 1.0f, 1.0f);
        clickEarth /= clickEarth.w; // important! perpective matrix may not be normalized
        return glm::intersectRaySphere(cameraEarth, glm::normalize(vec3(clickEarth) - cameraEarth),
                                       vec3(0.0f), radius, position, normal);
    };
    
    auto didClickEarth = [&] (const vec3& position) {
        if (shouldRotate) {
            float angle = glm::angle(lastIntersect, position);
            if (angle > 3E-3) {
                rotAngle = angle;
                rotAxis = glm::cross(lastIntersect, position);
                rotateScene(rotAngle, rotAxis);
            } else {
                rotAngle = 0.0f;
            }
        } else {
            // update lastIntersect only at the first intersection
            // the intersection point on the earth should not change
            // until the click stops
            lastIntersect = position;
            shouldRotate = true;
            rotAngle = 0.0f; // important when a click happens during inertial scrolling
        }
    };
    
    auto scrollOnCondition = [&] () {
        if (shouldScroll) {
            float elapsedTime = glfwGetTime() - scrollStartTime;
            if (rotAngle == 0.0f || elapsedTime > INERTIAL_COEFF) {
                shouldScroll = false;
            } else {
                elapsedTime /= INERTIAL_COEFF;
                float frac = (1.0f - elapsedTime * elapsedTime);
                rotateScene(rotAngle * frac, rotAxis);
            }
        } else {
            // start scrolling if did rotate at the last frame, and:
            // (1) the click point is out of the earth at this frame, or
            // (2) the mouse key is released at this frame
            if (shouldRotate) {
                shouldScroll = true;
                scrollStartTime = glfwGetTime();
            }
            shouldRotate = false;
        }
    };
    
    while (!glfwWindowShouldClose(window)) {
        processKeyboardInput();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (shouldUpdateCamera) {
            shouldUpdateCamera = false;
            updateCamera();
        }
        
        bool mayScroll = true, mayOnCurve = false;
        if (isClicking) {
            updateMousePos();
            vec3 position, normal;
            bool didIntersect = getIntersection(position, normal);
            
            if (!enableRotation) { // editing mode
                // need the intersection point on the atmosphere instead
                getIntersection(position, normal, AURORA_RELA_HEIGHT);
                spline.processMouseClick(!didClickRight, position, clickNDC, sideLengthNDC, glm::inverse(ndcToEarth));
                mayOnCurve = true;
            } else if (didIntersect) {
                didClickEarth(position);
                mayScroll = false;
            }
        }
        
        if (didClickRight) isClicking = didClickRight = false;
        
        if (!mayOnCurve) spline.deselectControlPoint();
        
        // not consider inertial scrolling only when
        // rotation is enabled and intersection is detected
        if (mayScroll) scrollOnCondition();
        else shouldScroll = false; // stop inertial scrolling
        
        renderScene();
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
