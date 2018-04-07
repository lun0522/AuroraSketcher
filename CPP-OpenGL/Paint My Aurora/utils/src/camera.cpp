//
//  camera.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/13/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

using std::runtime_error;
using glm::vec3;
using glm::mat4;

Camera::Camera(const vec3& position,
               const vec3& front,
               const vec3& up,
               const float fov,
               const float near,
               const float far,
               const float yaw,
               const float pitch,
               const float sensitivity):
position(position), front(front), up(up),
fov(fov), near(near), far(far), yaw(yaw), pitch(pitch),
sensitivity(sensitivity), firstFrame(true) {
    updateRight();
    updateViewMatrix();
}

void Camera::setScreenSize(const int screenWidth, const int screenHeight) {
    width = screenWidth;
    height = screenHeight;
    lastX = width / 2.0f;
    lastY = height / 2.0f;
    updateProjectionMatrix();
}

void Camera::processMouseMove(const double xPos, const double yPos) {
    if (firstFrame) {
        lastX = xPos;
        lastY = yPos;
        firstFrame = false;
    }
    
    float xOffset = (xPos - lastX) * sensitivity;
    float yOffset = (lastY - yPos) * sensitivity;
    lastX = xPos;
    lastY = yPos;
    
    yaw += xOffset;
    if (yaw >= 360.0f) yaw -= 360.0f;
    
    pitch += yOffset;
    if (pitch > 89.0f) pitch = 89.0f;
    else if (pitch < -89.0f) pitch = -89.0f;
    
    front = vec3(cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
                      sin(glm::radians(pitch)),
                      cos(glm::radians(pitch)) * sin(glm::radians(yaw)));
    updateRight();
    updateViewMatrix();
}

void Camera::processMouseScroll(const double yOffset) {
    fov += yOffset;
    if (fov < 1.0f) fov = 1.0f;
    else if (fov > 60.0f) fov = 60.0f;
    updateProjectionMatrix();
}

void Camera::processKeyboardInput(const CameraMoveDirection direction, const float deltaTime) {
    float speed = deltaTime * 10.0f;
    switch (direction) {
        case UP:
            position += front * speed;
            break;
        case DOWN:
            position -= front * speed;
            break;
        case LEFT:
            position += right * speed;
            break;
        case RIGHT:
            position -= right * speed;
            break;
        default:
        throw runtime_error("Invalid direction");
    }
    updateViewMatrix();
}

const glm::vec3& Camera::getPosition() const {
    return position;
}

const glm::vec3& Camera::getDirection() const {
    return front;
}

const mat4& Camera::getViewMatrix() const {
    return view;
}

const mat4& Camera::getProjectionMatrix() const {
    if (width == 0.0f || height == 0.0f) throw runtime_error("Screen size not set");
    return projection;
}

void Camera::updateRight() {
    right = glm::normalize(glm::cross(front, up));
}

void Camera::updateProjectionMatrix() {
    if (width == 0.0f || height == 0.0f) throw runtime_error("Screen size not set");
    projection = glm::perspective(glm::radians(fov), width / height, near, far);
}

void Camera::updateViewMatrix() {
    view = glm::lookAt(position, position + front, up);
}
