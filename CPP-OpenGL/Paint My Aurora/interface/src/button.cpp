//
//  button.cpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/14/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <glm/gtc/matrix_transform.hpp>

#include "loader.hpp"
#include "button.hpp"

using std::string;
using glm::vec2;
using glm::vec3;
using glm::mat4;

Button::Button(const string& text,
               const string& modelPath,
               const string& alphaMapPath,
               const string& vertexPath,
               const string& fragmentPath,
               const vec2& center,
               const vec2& size,
               const vec3& selectedColor,
               const vec3& unselectedColor,
               const bool selected):
button(Model(modelPath)), alphaMap(Loader::loadTexture(alphaMapPath, false)),
shader(Shader(vertexPath, fragmentPath)), selected(selected),
center(center * 2.0f - 1.0f), halfSize(size * 2.0f / 2.0f), // both in NDC
selectedColor(selectedColor), unselectedColor(unselectedColor) {
    mat4 modelMatrix(1.0f);
    modelMatrix = glm::translate(modelMatrix, vec3(this->center, 0.0f));
    modelMatrix = glm::scale(modelMatrix, vec3(halfSize * 2.0f, 1.0f));
    
    shader.use();
    shader.setInt("alphaMap", 0);
    shader.setMat4("model", modelMatrix);
    setColor();
}

void Button::setColor() {
    shader.use();
    shader.setFloat("alpha", selected ? 1.0f : 0.5f);
    shader.setVec3("color", selected ? selectedColor : unselectedColor);
}

bool Button::changeState() {
    selected = !selected;
    setColor();
    return selected;
}

bool Button::isHit(const glm::vec2& position) {
    vec2 toCenter = glm::abs(position - center);
    return glm::all(glm::lessThanEqual(toCenter, halfSize));
}

void Button::draw() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, alphaMap);
    button.draw(shader);
}
