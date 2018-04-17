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
               const string& alphaMapPath,
               const string& vertexPath,
               const string& fragmentPath,
               const vec2& center,
               const vec2& size,
               const vec3& selectedColor,
               const vec3& unselectedColor,
               const bool selected):
alphaMap(Loader::loadTexture(alphaMapPath, false)),
shader(Shader(vertexPath, fragmentPath)),
center(center * 2.0f - 1.0f), halfSize(size * 2.0f / 2.0f), // both in NDC
selectedColor(selectedColor), unselectedColor(unselectedColor), selected(selected) {
    shader.use();
    shader.setInt("alphaMap", 0);
    setVertexAttrib();
    setColor();
}

void Button::setVertexAttrib() {
    float vertexAttrib[] = {
        center.x + halfSize.x, center.y + halfSize.y,  1.0f, 1.0f,
        center.x - halfSize.x, center.y + halfSize.y,  0.0f, 1.0f,
        center.x - halfSize.x, center.y - halfSize.y,  0.0f, 0.0f,
        
        center.x + halfSize.x, center.y + halfSize.y,  1.0f, 1.0f,
        center.x - halfSize.x, center.y - halfSize.y,  0.0f, 0.0f,
        center.x + halfSize.x, center.y - halfSize.y,  1.0f, 0.0f,
    };
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrib), vertexAttrib, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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

bool Button::isHit(const glm::vec2& position) const {
    vec2 toCenter = glm::abs(position - center);
    return glm::all(glm::lessThanEqual(toCenter, halfSize));
}

void Button::draw() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, alphaMap);
    shader.use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
