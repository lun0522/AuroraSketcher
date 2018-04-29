//
//  button.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/14/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <vector>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

#include "button.hpp"

using std::string;
using std::vector;
using std::runtime_error;
using std::unordered_map;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using Character = Loader::Character;

Button::Button(const Shader& shader,
               const string& alphaMapPath,
               const vec2& buttonCenter,
               const vec2& buttonSize,
               const vec3& selectedColor,
               const vec3& unselectedColor,
               const bool selected):
shader(shader), alphaMap(Loader::loadTexture(alphaMapPath, false)), textLength(0),
center(buttonCenter * 2.0f - 1.0f), halfSize(buttonSize * 2.0f / 2.0f), // both in NDC
selectedColor(selectedColor), unselectedColor(unselectedColor), selected(selected) {
    float bgAttrib[] = {
        center.x + halfSize.x, center.y + halfSize.y,  1.0f, 1.0f,
        center.x - halfSize.x, center.y + halfSize.y,  0.0f, 1.0f,
        center.x - halfSize.x, center.y - halfSize.y,  0.0f, 0.0f,
        
        center.x + halfSize.x, center.y + halfSize.y,  1.0f, 1.0f,
        center.x - halfSize.x, center.y - halfSize.y,  0.0f, 0.0f,
        center.x + halfSize.x, center.y - halfSize.y,  1.0f, 0.0f,
    };
    glGenVertexArrays(1, &bgVAO);
    glBindVertexArray(bgVAO);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgAttrib), bgAttrib, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Button::setText(const string& text,
                     const vec2& scale,
                     const float yOffset,
                     const GLuint texture,
                     const vec3& color,
                     const unordered_map<char, Character>& charFrame) {
    textLength = (int) text.size();
    textTex = texture;
    textColor = color;
    
    vector<float> textAttrib;
    textAttrib.reserve(textLength * 6 * 4);
    float xOffset = 0.0f;
    for (const char& c : text) {
        auto cf = charFrame.find(c);
        if (cf == charFrame.end()) throw runtime_error("Failed to find char " + std::to_string(c));
        
        // https://learnopengl.com/In-Practice/Text-Rendering
        const Character& frame = cf->second;
        float xPos = center.x + xOffset + frame.bearing.x * scale.x;
        float yPos = center.y + yOffset - (frame.size.y - frame.bearing.y) * scale.y;
        vec2 size = vec2(frame.size) * scale;
        xOffset += frame.advance * scale.x;
        
        float charAttrib[6][4] {
            { xPos,          yPos + size.y,  frame.originX,                  frame.extent.y },
            { xPos,          yPos,           frame.originX,                  0.0f           },
            { xPos + size.x, yPos,           frame.originX + frame.extent.x, 0.0f           },
            
            { xPos,          yPos + size.y,  frame.originX,                  frame.extent.y },
            { xPos + size.x, yPos,           frame.originX + frame.extent.x, 0.0f           },
            { xPos + size.x, yPos + size.y,  frame.originX + frame.extent.x, frame.extent.y },
        };
        textAttrib.insert(textAttrib.end(), *charAttrib, *charAttrib + 6 * 4);
    }
    // let x coordiante subtract half of the total width, to achieve center alignment
    xOffset /= 2.0f;
    for (int i = 0; i < textAttrib.size(); i += 4)
        textAttrib[i] -= xOffset;
    
    glGenVertexArrays(1, &textVAO);
    glBindVertexArray(textVAO);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, textAttrib.size() * sizeof(float), textAttrib.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool Button::changeState() {
    selected = !selected;
    return selected;
}

bool Button::isHit(const vec2& position) const {
    vec2 toCenter = glm::abs(position - center);
    return glm::all(glm::lessThanEqual(toCenter, halfSize));
}

void Button::draw() const {
    // shader is shared by button background and text
    shader.use();
    shader.setInt("texture0", 0);
    shader.setFloat("depth", -0.99);
    shader.setFloat("alpha", selected ? 1.0f : 0.5f);
    shader.setVec3("color", selected ? selectedColor : unselectedColor);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, alphaMap);
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    if (textLength > 0) {
        shader.setInt("texture0", 1);
        shader.setFloat("depth", -1.0);
        shader.setFloat("alpha", selected ? 1.0f : 0.2f);
        shader.setVec3("color", textColor);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textTex);
        glBindVertexArray(textVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6 * textLength);
    }
    
    // clean up
    glBindVertexArray(0);
}
