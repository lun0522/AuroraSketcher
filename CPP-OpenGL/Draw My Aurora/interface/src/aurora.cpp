//
//  aurora.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/25/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include "loader.hpp"
#include "aurora.hpp"

using std::vector;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using uchar = unsigned char;

static const int DISTANCE_FIELD_SIZE = 800;
static const float MIN_FOV = 15.0f;
static const float MAX_FOV = 45.0f;

Aurora::Aurora(const GLuint prevFrameBuffer,
               const float fov,
               const float yaw,
               const float pitch,
               const float sensitivity):
fov(fov), yaw(yaw), pitch(pitch), sensitivity(sensitivity),
pathShader("path.vs", "path.fs"), auroraShader("aurora.vs", "aurora.fs"),
field(DistanceField(DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE)) {
    // distance field will be stored in this image
    image = (uchar *)malloc(DISTANCE_FIELD_SIZE * DISTANCE_FIELD_SIZE * sizeof(uchar));
    
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
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
    
    auroraShader.use();
    auroraShader.setInt("auroraDeposition", 0);
    auroraShader.setInt("auroraTexture", 1);
    auroraShader.setInt("distanceField", 2);
    deposition = Loader::loadTexture("deposition.jpg", false);
}

void Aurora::mainLoop(const vector<CRSpline>& splines,
                      const vec3& cameraPos,
                      const GLuint prevFrameBuffer,
                      const vec4& prevViewPort) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // draw all paths to framebuffer
    glLineWidth(10.0f);
    std::for_each(splines.begin(), splines.end(),
                  [&] (const CRSpline& spline) { spline.drawLine(pathShader); });
    glLineWidth(1.0f);
    
    // copy data to image
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // calculate distance field
    field.generate(image);
    GLuint fieldTex;
    glGenTextures(1, &fieldTex);
    glBindTexture(GL_TEXTURE_2D, fieldTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // render aurora
    auroraShader.use();
    auroraShader.setVec3("cameraPos", cameraPos);
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deposition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, fieldTex);
    
    shouldQuit = false;
    while (!shouldQuit) {
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
    glDeleteTextures(1, &fieldTex);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFrameBuffer);
    glViewport(prevViewPort.x, prevViewPort.y, prevViewPort.z, prevViewPort.w);
}

void Aurora::didScrollMouse(const double yOffset) {
    fov += yOffset;
    if (fov < MIN_FOV) fov = MIN_FOV;
    else if (fov > MAX_FOV) fov = MAX_FOV;
}

void Aurora::didMoveMouse(const vec2& position) {
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
    
    front = vec3(cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
                 sin(glm::radians(pitch)),
                 cos(glm::radians(pitch)) * sin(glm::radians(yaw)));
}

void Aurora::quit() {
    shouldQuit = true;
}

Aurora::~Aurora() {
    if (image) free(image);
}
