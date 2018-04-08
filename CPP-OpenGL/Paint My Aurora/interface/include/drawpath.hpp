//
//  drawpath.hpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef drawpath_hpp
#define drawpath_hpp

#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class DrawPath {
    std::string directory;
    GLFWwindow *window;
    void processKeyboardInput();
public:
    DrawPath(const char *directory);
    void mainLoop();
};

#endif /* drawpath_hpp */
