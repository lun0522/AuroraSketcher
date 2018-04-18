//
//  mesh.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/22/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <stdexcept>

#include "mesh.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<GLuint>& indices,
           const std::vector<Texture>& textures):
vertices(vertices), indices(indices), textures(textures) {
    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // GL_STATIC_DRAW: for data that does not change
    // GL_DYNAMIC_DRAW: for data that changes a lot
    // GL_STREAM_DRAW: for data that changes every time when it is drawn
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // enable attributes
    GLsizei stride = 8 * sizeof(GLfloat);
    // which vertex attribute to configure (location)
    // size of vertex attribute
    // data type
    // whether e want the data to be normalized
    // space between consecutive vertex attribute sets (stride) (can set to be 0 if tightly packed)
    // offset of the position data begins in the buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    
    // unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::bindTexture(const Shader& shader, const GLuint texOffset) const {
    int diffIdx = 0, specIdx = 0, reflIdx = 0;
    for (int i = 0; i < textures.size(); ++i) {
        std::string name;
        switch (textures[i].type) {
            case DIFFUSE:
                name = "material.diffuse" + std::to_string(diffIdx++);
                break;
            case SPECULAR:
                name = "material.specular" + std::to_string(specIdx++);
                break;
            case REFLECTION:
                name = "material.reflection" + std::to_string(reflIdx++);
                break;
            default:
                throw std::runtime_error("Unknown texture type");
        }
        GLuint texIndex = texOffset + i;
        glActiveTexture(GL_TEXTURE0 + texIndex);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
        shader.setInt(name, texIndex);
    }
}

void Mesh::draw(const Shader& shader,
                const GLuint texOffset,
                const bool loadTexture) const {
    if (loadTexture) bindTexture(shader, texOffset);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::drawInstanced(const Shader& shader,
                         const GLuint amount,
                         const GLuint texOffset,
                         const bool loadTexture) const {
    if (loadTexture) bindTexture(shader, texOffset);
    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, amount);
    glBindVertexArray(0);
}
