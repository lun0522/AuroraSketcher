//
//  drawpath.cpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "shader.hpp"
#include "loader.hpp"
#include "model.hpp"
#include "drawpath.hpp"

using std::string;
using std::vector;
using std::runtime_error;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

static const float EARTH_RADIUS = 6378.1f;
static const float AURORA_HEIGHT = 100.0f;
static const float AURORA_RELA_HEIGHT = (EARTH_RADIUS + AURORA_HEIGHT) / EARTH_RADIUS;
static const float CTRL_POINT_SIDE_LENGTH = 20.0f;
static const float CLICK_CTRL_POINT_TOLERANCE = 10.0f;
static const float INERTIAL_COEFF = 1.5f;
static const int NUM_AURORA_PATH = 3;
static const int NUM_BUTTON_BOTTOM = 3;
static const int NUM_BUTTON_TOTAL = NUM_AURORA_PATH + NUM_BUTTON_BOTTOM;
static const int BUTTON_NOT_HIT = -1;
static const vec3 CAMERA_POS(0.0f, 0.0f, 30.0f);

void DrawPath::didClickMouse(const bool isLeft, const bool isPress) {
    if (isLeft) {
        if (isPress) didClickLeft = true;
        wasClicking = isPress;
    } else {
        if (isPress) didClickRight = true;
    }
}

void DrawPath::didScrollMouse(const float yPos) {
    camera.processMouseScroll(yPos, 15.0f, 45.0f);
    shouldUpdateCamera = true;
}

void DrawPath::didPressUpOrDown(const bool isUp) {
    isDay = isUp;
}

void DrawPath::didPressButton(const int index) {
    switch (index) {
        case 0: // editing
            buttons[0].changeState();
            isEditing = !isEditing;
            break;
        case 1: // daylight
            buttons[1].changeState();
            isDay = !isDay;
            break;
        case 2: // aurora
            // temporarily empty
            break;
        default: // path
            int pathIndex = index - NUM_BUTTON_BOTTOM;
            if (pathIndex != editingPath) {
                buttons[editingPath + NUM_BUTTON_BOTTOM].changeState();
                buttons[index].changeState();
                splines[editingPath].deselectControlPoint();
                editingPath = pathIndex;
            }
            break;
    }
}

DrawPath::DrawPath(const char *directory):
directory(directory), window(this), camera(CAMERA_POS) {
    camera.setScreenSize(window.getOriginalSize());
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
    
    Shader buttonShader(directory + "interface/shaders/button.vs",
                        directory + "interface/shaders/button.fs");
    vector<vec3> buttonColor {
        vec3( 52, 152, 219), vec3( 41, 128, 185),
        vec3(155,  89, 182), vec3(142,  68, 173),
        vec3( 46, 204, 113), vec3( 39, 174,  96),
        vec3(241, 196,  15), vec3(243, 156,  18),
        vec3(230, 126,  34), vec3(211,  84,   0),
        vec3(231,  76,  60), vec3(192,  57,  43),
    };
    std::for_each(buttonColor.begin(), buttonColor.end(),
                  [] (vec3& color) { color /= 255.0f; });
    vector<string> buttonText {
        "Editing",
        "Daylight",
        "Aurora",
        "Path 1",
        "Path 2",
        "Path 3",
    };
    std::unordered_map<char, Character> charFrame;
    GLuint textTex = Loader::loadCharacter(directory + "texture/button/ostrich.ttf",
                                           directory + "interface/shaders/character.vs",
                                           directory + "interface/shaders/character.fs",
                                           buttonText,
                                           charFrame,
                                           0,
                                           window.getViewPort());
    
    auto createButton = [&] (const int index, const vec2& center, const vec2& size) -> Button {
        Button button(buttonShader,
                      directory + "texture/button/rect_rounded.jpg",
                      center, size,
                      buttonColor[index * 2], buttonColor[index * 2 + 1]);
        button.setText(buttonText[index], vec2(0.0015f), -0.03f, textTex, vec3(1.0f), charFrame);
        return button;
    };
    for (int i = 0; i < NUM_BUTTON_TOTAL; ++i) {
        float centerX = 0.5f + 0.31f * (i % 3 - 1.0f);
        float centerY = i < NUM_BUTTON_BOTTOM ? 0.06f : 0.94f;
        buttons.push_back(createButton(i, vec2(centerX, centerY), vec2(0.2f, 0.06f)));
    }
    
    
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
    vector<string> boxfaces {
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
    
    Shader pointShader(directory + "interface/shaders/spline.vs",
                       directory + "interface/shaders/spline.fs",
                       directory + "interface/shaders/spline.gs");
    pointShader.use();
    vec2 sideLengthNDC = vec2(CTRL_POINT_SIDE_LENGTH) / window.getOriginalSize() * 2.0f;
    pointShader.setVec2("sideLength", sideLengthNDC);
    sideLengthNDC *= (CTRL_POINT_SIDE_LENGTH + CLICK_CTRL_POINT_TOLERANCE) / CTRL_POINT_SIDE_LENGTH;
    
    Shader curveShader(directory + "interface/shaders/spline.vs",
                       directory + "interface/shaders/spline.fs");
    
    vector<float> latitude = { 60.0f, 70.0f, 80.0f };
    for (int i = 0; i < NUM_AURORA_PATH; ++i) {
        vector<vec3> controlPoints;
        float lat = glm::radians(latitude[i]);
        float sinLat = sin(lat), cosLat = cos(lat);
        for (float angle = 0.0f; angle < 360.0f; angle += 45.0f)
            controlPoints.push_back(vec3(cos(glm::radians(angle)) * cosLat, sinLat, sin(glm::radians(angle)) * cosLat));
        
        splines.push_back(CRSpline(pointShader, curveShader,
                                   buttonColor[(i + NUM_BUTTON_BOTTOM) * 2],
                                   controlPoints, AURORA_RELA_HEIGHT));
    }
    
    // the first path is chosen by default
    editingPath = 0;
    buttons[NUM_BUTTON_BOTTOM + 0].changeState();
    
    
    // ------------------------------------
    // render
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    int frameCount = 0;
    float rotAngle = 0.0f, scrollStartTime = 0.0f, lastTime = glfwGetTime();
    mat4 worldToNDC, ndcToWorld, ndcToEarth, worldToEarth = glm::inverse(earthModel);
    vec3 lastIntersect, rotAxis, cameraEarth = worldToEarth * vec4(CAMERA_POS, 1.0);
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
        glBindTexture(GL_TEXTURE_2D, isDay ? earthDayTex : earthNightTex);
        earthShader.setInt("earthMap", 0);
        earth.draw(earthShader);

        std::for_each(splines.begin(), splines.end(),
                      [] (CRSpline& spline) { spline.draw(); });

        glDepthFunc(GL_LEQUAL);
        universeShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, universeTex);
        universeShader.setInt("cubemap", 0);
        universe.draw(universeShader);
        glDepthFunc(GL_LESS);
        
        // render buttons at last because of alpha blending
        int numDisplay = isEditing ? NUM_BUTTON_TOTAL : NUM_BUTTON_BOTTOM;
        std::for_each(buttons.begin(), buttons.begin() + numDisplay,
                      [] (Button& button) { button.draw(); });
    };
    
    auto rotateScene = [&] (const float angle, const vec3& axis) {
        earthModel = glm::rotate(earthModel, angle, axis);
        worldToEarth = glm::inverse(earthModel);
        ndcToEarth = worldToEarth * ndcToWorld;
        cameraEarth = worldToEarth * vec4(CAMERA_POS, 1.0);
        
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), glm::value_ptr(earthModel));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        
        universeModel = glm::rotate(universeModel, angle, axis);
        universeShader.use();
        universeShader.setMat4("model", universeModel);
    };
    
    auto didHitButton = [&] () -> int {
        int hitButton = BUTTON_NOT_HIT;
        int numTest = isEditing ? NUM_BUTTON_TOTAL : NUM_BUTTON_BOTTOM;
        for (int i = 0; i < numTest; ++i) {
            if (buttons[i].isHit(window.getClickNDC())) {
                hitButton = i;
                break;
            }
        }
        return hitButton;
    };
    
    auto getIntersection = [&] (vec3& position, vec3& normal, const float radius = 1.0f) -> bool {
        vec4 clickEarth = ndcToEarth * vec4(window.getClickNDC(), 1.0f, 1.0f);
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
    
    while (!window.shouldClose()) {
        window.processKeyboardInput();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (shouldUpdateCamera) {
            shouldUpdateCamera = false;
            updateCamera();
        }
        
        if (wasClicking || didClickLeft || didClickRight)
            window.updateMousePos();
        
        bool mayScroll = true, mayOnSpline = false, stillClicking = false;
        if (didClickRight) {
            // right click is valid only in edit mode, and the click point is on the atmosphere
            // button hit test is done before intersection detection, to save some computation
            if (isEditing) {
                int hitButton = didHitButton();
                if (hitButton == BUTTON_NOT_HIT) {
                    vec3 position, normal;
                    if (getIntersection(position, normal, AURORA_RELA_HEIGHT)) {
                        splines[editingPath].processMouseClick(false, position, window.getClickNDC(), sideLengthNDC, glm::inverse(ndcToEarth));
                        mayOnSpline = true;
                    }
                }
            }
        } else if (wasClicking || didClickLeft) {
            // left click on buttons is always valid
            int hitButton = didHitButton();
            if (hitButton != BUTTON_NOT_HIT) {
                didPressButton(hitButton);
            } else {
                // in edit mode, right click is valid if the click point is on the atmosphere
                // while in rotate mode, click point should be on the surface of earth
                vec3 position, normal;
                if (isEditing) {
                    if (getIntersection(position, normal, AURORA_RELA_HEIGHT)) {
                        splines[editingPath].processMouseClick(true, position, window.getClickNDC(), sideLengthNDC, glm::inverse(ndcToEarth));
                        // do not simply assign true to stillClicking!
                        // left mouse key may have already been released
                        stillClicking = wasClicking;
                        mayOnSpline = true;
                    }
                } else {
                    if (getIntersection(position, normal)) {
                        didClickEarth(position);
                        stillClicking = wasClicking;
                        mayScroll = false;
                    }
                }
            }
        }
        
        didClickLeft = didClickRight = false;
        wasClicking = stillClicking; // reserve the state of left clicking for the next frame
        
        if (!mayOnSpline) splines[editingPath].deselectControlPoint();
        
        // not consider inertial scrolling only when
        // rotation is enabled and intersection is detected
        if (mayScroll) scrollOnCondition();
        else shouldScroll = false; // stop inertial scrolling
        
        renderScene();
        window.renderFrame();
        
        ++frameCount;
        float currentTime = glfwGetTime();
        if (currentTime - lastTime > 1.0) {
            std::cout <<  "FPS: " << std::to_string(frameCount) << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }
}
