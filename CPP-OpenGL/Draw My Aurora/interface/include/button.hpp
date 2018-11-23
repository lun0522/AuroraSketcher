//
//  button.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/14/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef button_hpp
#define button_hpp

#include <string>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "loader.hpp"

class Shader;

class Button {
    const Shader& shader;
    GLuint bgVAO, textVAO, textTex, alphaMap;
    int textLength;
    bool selected;
    glm::vec2 center, halfSize;
    glm::vec3 selectedColor, unselectedColor, textColor;
public:
    Button(const Shader& shader,
           const std::string& alphaMapPath,
           const glm::vec2& buttonCenter,
           const glm::vec2& buttonSize,
           const glm::vec3& selectedColor,
           const glm::vec3& unselectedColor);
    void setText(const std::string& text,
                 const glm::vec2& scale,
                 const float yOffset,
                 const GLuint texture,
                 const glm::vec3& color,
                 const std::unordered_map<char, Loader::Character>& charFrame);
    bool changeState();
    bool isHit(const glm::vec2& position) const;
    void draw() const;
};

#endif /* button_hpp */
