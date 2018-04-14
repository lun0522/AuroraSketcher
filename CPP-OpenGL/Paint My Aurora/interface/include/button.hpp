//
//  button.hpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/14/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef button_hpp
#define button_hpp

#include <string>
#include <glm/glm.hpp>

#include "model.hpp"
#include "shader.hpp"

class Button {
    Model button;
    Shader shader;
    GLuint alphaMap;
    bool selected;
    glm::vec2 center, halfSize;
    glm::vec3 selectedColor, unselectedColor;
    void setColor();
public:
    Button(const std::string& text,
           const std::string& modelPath,
           const std::string& alphaMapPath,
           const std::string& vertexPath,
           const std::string& fragmentPath,
           const glm::vec2& center,
           const glm::vec2& size,
           const glm::vec3& selectedColor,
           const glm::vec3& unselectedColor,
           const bool selected = false);
    void setSelected(const bool value);
    bool isHit(glm::vec2& position);
    void draw();
};

#endif /* button_hpp */
