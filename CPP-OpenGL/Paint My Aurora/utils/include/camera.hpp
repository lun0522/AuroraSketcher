//
//  camera.hpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/13/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef camera_hpp
#define camera_hpp

#include <glm/glm.hpp>

enum CameraMoveDirection { UP, DOWN, LEFT, RIGHT };

class Camera {
    glm::vec3 position, front, up, right;
    glm::mat4 view, projection;
    float fov, near, far, yaw, pitch, width, height, lastX, lastY, sensitivity;
    bool firstFrame;
    void updateRight();
    void updateViewMatrix();
    void updateProjectionMatrix();
public:
    Camera(const glm::vec3& position = glm::vec3(0.0f, 0.0f, 0.0f),
           const glm::vec3& front = glm::vec3(0.0f, 0.0f, -1.0f),
           const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f),
           const float fov = 45.0f,
           const float near = 0.1f,
           const float far = 100.0f,
           const float yaw = -90.0f,
           const float pitch = 0.0f,
           const float sensitivity = 0.05f);
    void setScreenSize(const int screenWidth, const int screenHeight);
    void processMouseMove(const double xPos, const double yPos);
    void processMouseScroll(const double yOffset, const double minVal, const double maxVal);
    void processKeyboardInput(const CameraMoveDirection direction, const float distance);
    const glm::vec3& getPosition() const;
    const glm::vec3& getDirection() const;
    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getProjectionMatrix() const;
};

#endif /* camera_hpp */
