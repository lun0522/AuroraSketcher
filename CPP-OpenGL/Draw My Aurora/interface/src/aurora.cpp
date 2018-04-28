//
//  aurora.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/25/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>

#include "loader.hpp"
#include "aurora.hpp"

using std::vector;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

static const int DISTANCE_FIELD_SIZE = 2048;
static const int PATH_NUM_SAMPLE = 8;
static const float AURORA_WIDTH = 4.0f;
static const float MIN_FOV = 10.0f;
static const float MAX_FOV = 60.0f;

Aurora::Aurora(const GLuint prevFrameBuffer,
               const float fov,
               const float yaw,
               const float pitch,
               const float sensitivity):
originFov(fov), originYaw(yaw), originPitch(pitch), sensitivity(sensitivity),
pathLineShader("path.vs", "path.fs", "path.gs"),
pathPointsShader("path.vs", "path.fs"),
auroraShader("aurora.vs", "aurora.fs"),
field(DistanceField(DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE)) {
    // distance field will be stored in this image
    image = (unsigned char *)malloc(DISTANCE_FIELD_SIZE * DISTANCE_FIELD_SIZE * sizeof(unsigned char));
    
    glGenTextures(1, &pathTex);
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFrameBuffer);
    
    // vertices for ray tracer
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    float vertices[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, 1.0f,   1.0f, -1.0f,  1.0f,  1.0f,
    };
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    pathLineShader.use();
    pathLineShader.setFloat("lineWidth", AURORA_WIDTH / DISTANCE_FIELD_SIZE);
    
    auroraShader.use();
    auroraShader.setInt("auroraDeposition", 0);
    auroraShader.setInt("auroraTexture", 1);
    auroraShader.setInt("distanceField", 2);
    deposition = Loader::loadTexture("deposition.jpg", true);
}

void Aurora::generatePath(const std::vector<CRSpline> &splines,
                          const GLuint prevFrameBuffer,
                          const vec4& prevViewPort) {
    // multisampling
    GLuint pathMultiSample;
    glGenTextures(1, &pathMultiSample);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, pathMultiSample);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, PATH_NUM_SAMPLE, GL_RED, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GLuint framebufferMultiSample;
    glGenFramebuffers(1, &framebufferMultiSample);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMultiSample);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, pathMultiSample, 0);
    
    // render all paths to multisample framebuffer
    glViewport(0, 0, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE);
    glPointSize(AURORA_WIDTH / 2.0f);
    std::for_each(splines.begin(), splines.end(), [&] (const CRSpline& spline) {
        spline.draw(pathLineShader, GL_LINE_STRIP);
        spline.draw(pathPointsShader, GL_POINTS);
    });
    
    // down sampling
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferMultiSample);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    glBlitFramebuffer(0, 0, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE,
                      0, 0, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glDeleteTextures(1, &pathMultiSample);
    glDeleteFramebuffers(1, &framebufferMultiSample);
    
    // copy data to image
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // calculate distance field
    field.generate(image);
    glGenTextures(1, &fieldTex);
    glBindTexture(GL_TEXTURE_2D, fieldTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindFramebuffer(GL_FRAMEBUFFER, prevFrameBuffer);
    glViewport(prevViewPort.x, prevViewPort.y, prevViewPort.z, prevViewPort.w);
}

void Aurora::mainLoop(const Window& window,
                      const vec3& cameraPos,
                      const vec2& screenSize,
                      const vector<CRSpline>& splines,
                      const GLuint prevFrameBuffer,
                      const vec4& prevViewPort) {
    generatePath(splines, prevFrameBuffer, prevViewPort);
    
    window.setCaptureCursor(true);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deposition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, fieldTex);
    
    float ratio = screenSize.x / screenSize.y;
    fov = originFov;
    yaw = originYaw;
    pitch = originPitch;
    vec3 cameraOrigin = glm::normalize(cameraPos);
    vec3 normal = cameraOrigin;
    // originally look at the north
    vec3 originDir = glm::normalize(vec3(0.0f, 1.0f / cameraOrigin.y, 0.0f) - cameraPos);
    // transfrom from orignial camera space to world space
    mat4 toWorld = glm::inverse(glm::lookAt(cameraPos, cameraPos + originDir, normal));
    auroraShader.use();
    auroraShader.setVec3("cameraPos", cameraPos);
    
    firstFrame = true;
    isRendering = true;
    shouldUpdate = true;
    shouldQuit = false;
    
    int frameCount = 0;
    float lastTime = glfwGetTime();
    
    while (!shouldQuit && !window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT);
        if (shouldUpdate) {
            shouldUpdate = false;
            // front and right are in world space
            // rotate using pitch and yaw in original camera space at first,
            // and then convert to world space
            vec3 front = vec3(cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
                              sin(glm::radians(pitch)),
                              cos(glm::radians(pitch)) * sin(glm::radians(yaw)));
            front = vec3(toWorld * vec4(front, 0.0f));
            vec3 right = glm::cross(front, normal);
            float zoom = tan(glm::radians(fov / 2.0f));
            auroraShader.setVec3("origin", cameraOrigin + front);
            auroraShader.setVec3("xAxis", right * ratio * zoom);
            auroraShader.setVec3("yAxis", glm::cross(right, front) * zoom);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);
        window.renderFrame();
        window.processKeyboardInput();
        
        ++frameCount;
        float currentTime = glfwGetTime();
        if (currentTime - lastTime > 1.0) {
            std::cout <<  "FPS: " << std::to_string(frameCount) << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }
    
    isRendering = false;
    shouldUpdate = false;
    window.setCaptureCursor(false);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glDeleteTextures(1, &fieldTex);
}

void Aurora::didScrollMouse(const double yOffset) {
    if (isRendering) {
        fov += yOffset;
        if (fov < MIN_FOV) fov = MIN_FOV;
        else if (fov > MAX_FOV) fov = MAX_FOV;
        shouldUpdate = true;
    }
}

void Aurora::didMoveMouse(const vec2& position) {
    if (isRendering) {
        if (firstFrame) {
            lastPos = position;
            firstFrame = false;
            return;
        }
        
        vec2 offset = (position - lastPos) * sensitivity;
        lastPos = position;
        
        yaw += offset.x;
        yaw = glm::mod(yaw, 360.0f);
        
        pitch -= offset.y;
        if (pitch > 89.0f) pitch = 89.0f;
        else if (pitch < -89.0f) pitch = -89.0f;
        
        shouldUpdate = true;
    }
}

void Aurora::quit() {
    shouldQuit = true;
}

Aurora::~Aurora() {
    if (image) free(image);
}
