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

class DrawPath;

class Window {
    GLFWwindow *window;
public:
    Window(DrawPath *drawPath);
    const glm::vec4& getViewPort() const;
    const glm::vec2& getOriginalSize() const;
    const glm::vec2& getClickNDC() const;
    void updateMousePos();
    void setCaptureCursor(const bool value) const;
    void processKeyboardInput() const;
    void renderFrame() const;
    bool shouldClose() const;
};

#endif /* window_hpp */
