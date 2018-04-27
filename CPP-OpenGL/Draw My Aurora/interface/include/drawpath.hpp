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
#include "aurora.hpp"
#include "crspline.hpp"

class DrawPath {
    Window window;
    Camera camera;
    Aurora aurora;
    std::vector<Button> buttons;
    std::vector<CRSpline> splines;
    int editingPath;
    bool isDay, isEditing, shouldUpdateCamera, shouldRenderAurora;
    bool wasClicking, didClickLeft, didClickRight;
public:
    DrawPath();
    void didClickMouse(const bool isLeft, const bool isPress);
    void didScrollMouse(const float yOffset);
    void didMoveMouse(const glm::vec2& position);
    void didPressButton(const int index);
    void mainLoop();
};

#endif /* drawpath_hpp */
