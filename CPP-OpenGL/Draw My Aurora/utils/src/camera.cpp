//
//  camera.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/13/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include "camera.hpp"

#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

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

void Camera::setScreenSize(const vec2& size) {
    screenSize = size;
    updateProjectionMatrix();
}

void Camera::processMouseMove(const vec2& position) {
    if (firstFrame) {
        lastPos = position;
        firstFrame = false;
        return;
    }
    
    vec2 offset = (position - lastPos) * sensitivity;
    lastPos = position;
    
    yaw += offset.x;
    yaw = mod(yaw, 360.0f);
    
    pitch -= offset.y;
    if (pitch > 89.0f) pitch = 89.0f;
    else if (pitch < -89.0f) pitch = -89.0f;
    
    front = vec3(cos(radians(pitch)) * cos(radians(yaw)),
                 sin(radians(pitch)),
                 cos(radians(pitch)) * sin(radians(yaw)));
    updateRight();
    updateViewMatrix();
}

void Camera::processMouseScroll(const double yOffset, const double minVal, const double maxVal) {
    fov += yOffset;
    if (fov < minVal) fov = minVal;
    else if (fov > maxVal) fov = maxVal;
    updateProjectionMatrix();
}

void Camera::processKeyboardInput(const Move direction, const float distance) {
    switch (direction) {
        case Move::up:
            position += front * distance;
            break;
        case Move::down:
            position -= front * distance;
            break;
        case Move::left:
            position += right * distance;
            break;
        case Move::right:
            position -= right * distance;
            break;
        default:
        throw runtime_error("Invalid direction");
    }
    updateViewMatrix();
}

const vec3& Camera::getPosition() const {
    return position;
}

const vec3& Camera::getDirection() const {
    return front;
}

const mat4& Camera::getViewMatrix() const {
    return view;
}

const mat4& Camera::getProjectionMatrix() const {
    if (any(equal(screenSize, vec2(0.0f)))) throw runtime_error("Screen size not set");
    return projection;
}

void Camera::updateRight() {
    right = normalize(cross(front, up));
}

void Camera::updateProjectionMatrix() {
    if (any(equal(screenSize, vec2(0.0f)))) throw runtime_error("Screen size not set");
    projection = perspective(radians(fov), screenSize.x / screenSize.y, near, far);
}

void Camera::updateViewMatrix() {
    view = lookAt(position, position + front, up);
}
