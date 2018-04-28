//
//  aurora.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/25/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef aurora_hpp
#define aurora_hpp

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "window.hpp"
#include "shader.hpp"
#include "crspline.hpp"
#include "distfield.hpp"

class Aurora {
    Shader pathLineShader, pathPointsShader, auroraShader;
    DistanceField field;
    unsigned char* image;
    GLuint VAO, pathTex, fieldTex, deposition, framebuffer;
    bool firstFrame, isRendering, shouldUpdate, shouldQuit;
    const float originFov, originYaw, originPitch;
    float fov, yaw, pitch, sensitivity;
    glm::vec2 lastPos;
    void generatePath();
    void generatePath(const std::vector<CRSpline>& splines,
                      const GLuint prevFrameBuffer,
                      const glm::vec4& prevViewPort);
public:
    Aurora(const GLuint prevFrameBuffer,
           const float fov = 45.0f,
           const float yaw = -90.0f,
           const float pitch = 0.0f,
           const float sensitivity = 0.05f);
    void mainLoop(const Window& window,
                  const glm::vec3& cameraPos,
                  const glm::vec2& screenSize,
                  const std::vector<CRSpline>& splines,
                  const GLuint prevFrameBuffer,
                  const glm::vec4& prevViewPort);
    void didScrollMouse(const double yOffset);
    void didMoveMouse(const glm::vec2& position);
    void quit();
    ~Aurora();
};

#endif /* aurora_hpp */
