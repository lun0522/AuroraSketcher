//
//  drawpath.hpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef drawpath_hpp
#define drawpath_hpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class DrawPath {
    GLFWwindow *window;
    void processKeyboardInput();
public:
    DrawPath();
    void mainLoop();
};

#endif /* drawpath_hpp */
