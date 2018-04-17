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
    Shader shader;
    GLuint VAO, alphaMap;
    bool selected;
    glm::vec2 center, halfSize;
    glm::vec3 selectedColor, unselectedColor;
    void setColor();
    void setVertexAttrib();
public:
    Button(const std::string& text,
           const std::string& alphaMapPath,
           const std::string& vertexPath,
           const std::string& fragmentPath,
           const glm::vec2& center,
           const glm::vec2& size,
           const glm::vec3& selectedColor,
           const glm::vec3& unselectedColor,
           const bool selected = false);
    bool changeState();
    bool isHit(const glm::vec2& position) const;
    void draw() const;
};

#endif /* button_hpp */
