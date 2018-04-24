//
//  drawpath.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/6/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef drawpath_hpp
#define drawpath_hpp

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "window.hpp"
#include "camera.hpp"
#include "button.hpp"
#include "crspline.hpp"

class DrawPath {
    Window window;
    Camera camera;
    std::vector<Button> buttons;
    std::vector<CRSpline> splines;
    int editingPath;
    bool isDay, isEditing, shouldUpdateCamera;
    bool wasClicking, didClickLeft, didClickRight;
public:
    DrawPath();
    void didClickMouse(const bool isLeft, const bool isPress);
    void didScrollMouse(const float yPos);
    void didPressButton(const int index);
    void mainLoop();
};

#endif /* drawpath_hpp */
