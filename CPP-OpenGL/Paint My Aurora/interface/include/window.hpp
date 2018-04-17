//
//  window.hpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/15/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef window_hpp
#define window_hpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class DrawPath;

class Window {
    GLFWwindow *window;
public:
    Window(DrawPath *drawPath);
    const glm::vec4& getViewPort();
    const glm::vec2& getOriginalSize();
    const glm::vec2& getClickNDC();
    void updateMousePos();
    void processKeyboardInput();
    void renderFrame();
    bool shouldClose();
};

#endif /* window_hpp */
