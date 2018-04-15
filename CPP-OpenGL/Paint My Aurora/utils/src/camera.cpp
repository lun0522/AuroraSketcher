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
using glm::vec2;
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

void Camera::setScreenSize(const glm::vec2& size) {
    screenSize = size;
    updateProjectionMatrix();
}

void Camera::processMouseMove(const glm::vec2& position) {
    if (firstFrame) {
        lastPos = position;
        firstFrame = false;
        return;
    }
    
    vec2 offset = (position - lastPos) * sensitivity;
    lastPos = position;
    
    yaw += offset.x;
    if (yaw >= 360.0f) yaw -= 360.0f;
    
    pitch -= offset.y;
    if (pitch > 89.0f) pitch = 89.0f;
    else if (pitch < -89.0f) pitch = -89.0f;
    
    front = vec3(cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
                      sin(glm::radians(pitch)),
                      cos(glm::radians(pitch)) * sin(glm::radians(yaw)));
    updateRight();
    updateViewMatrix();
}

void Camera::processMouseScroll(const double yOffset, const double minVal, const double maxVal) {
    fov += yOffset;
    if (fov < minVal) fov = minVal;
    else if (fov > maxVal) fov = maxVal;
    updateProjectionMatrix();
}

void Camera::processKeyboardInput(const CameraMoveDirection direction, const float distance) {
    switch (direction) {
        case UP:
            position += front * distance;
            break;
        case DOWN:
            position -= front * distance;
            break;
        case LEFT:
            position += right * distance;
            break;
        case RIGHT:
            position -= right * distance;
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
    if (glm::any(glm::equal(screenSize, vec2(0.0f)))) throw runtime_error("Screen size not set");
    return projection;
}

void Camera::updateRight() {
    right = glm::normalize(glm::cross(front, up));
}

void Camera::updateProjectionMatrix() {
    if (glm::any(glm::equal(screenSize, vec2(0.0f)))) throw runtime_error("Screen size not set");
    projection = glm::perspective(glm::radians(fov), screenSize.x / screenSize.y, near, far);
}

void Camera::updateViewMatrix() {
    view = glm::lookAt(position, position + front, up);
}
