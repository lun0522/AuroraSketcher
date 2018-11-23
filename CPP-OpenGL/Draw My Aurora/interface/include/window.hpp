//
//  window.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/15/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef window_hpp
#define window_hpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class DrawPath;

class Window {
    GLFWwindow *window;
    glm::vec2 originalSize, currentSize, retinaRatio;
    glm::vec2 posOffset, clickNDC;
    glm::vec4 viewPort;
public:
    Window(DrawPath *drawPath, int width = 800, int height = 600);
    const glm::vec4& getViewPort() const;
    const glm::vec2& getOriginalSize() const;
    const glm::vec2& getClickNDC() const;
    void setSize(int width, int height);
    void setViewPort(const glm::vec4& value);
    void updateMousePos();
    void setCaptureCursor(const bool value) const;
    void processKeyboardInput() const;
    void renderFrame() const;
    bool shouldClose() const;
};

#endif /* window_hpp */
