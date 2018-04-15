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
#include <glm/glm.hpp>

#include "window.hpp"
#include "camera.hpp"

class DrawPath {
    std::string directory;
    Window window;
    Camera camera;
    bool isDay, isEditing, shouldUpdateCamera;
    bool wasClicking, didClickLeft, didClickRight;
public:
    DrawPath(const char *directory);
    void didClickMouse(const bool isLeft, const bool isPress);
    void didScrollMouse(const float yPos);
    void didPressUpOrDown(const bool isUp);
    void didPressNumber(const int number);
    void mainLoop();
};

#endif /* drawpath_hpp */
