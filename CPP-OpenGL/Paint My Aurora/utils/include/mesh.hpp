//
//  mesh.hpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/22/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef mesh_hpp
#define mesh_hpp

#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "shader.hpp"

enum TextureType { DIFFUSE, SPECULAR, REFLECTION };

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Texture {
    GLuint id;
    TextureType type;
    std::string path;
};

class Mesh {
    GLuint VAO, VBO, EBO;
    const std::vector<Vertex> vertices;
    const std::vector<GLuint> indices;
    const std::vector<Texture> textures;
    void bindTexture(const Shader& shader, const GLuint texOffset) const;
public:
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<GLuint>& indices,
         const std::vector<Texture>& textures);
    void draw(const Shader& shader, const GLuint texOffset, const bool loadTexture) const;
    void drawInstanced(const Shader& shader, const GLuint amount, const GLuint texOffset, const bool loadTexture) const;
    template<typename Func>
    void appendData(Func& func) const {
        glBindVertexArray(VAO);
        func();
        glBindVertexArray(0);
    }
};

#endif /* mesh_hpp */
