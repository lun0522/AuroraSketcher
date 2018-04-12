//
//  shader.hpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/9/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef shader_hpp
#define shader_hpp

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
    GLuint programId;
    std::string readCode(std::string path);
public:
    Shader(const std::string& vertexPath,
           const std::string& fragmentPath,
           const std::string& geometryPath = "");
    void use() const;
    GLuint getUniform(const std::string &name) const;
    void setInt(const std::string &name, const int value) const;
    void setFloat(const std::string &name, const float value) const;
    void setVec2(const std::string &name, const glm::vec2& value) const;
    void setVec3(const std::string &name, const GLfloat v0, const GLfloat v1, const GLfloat v2) const;
    void setVec3(const std::string &name, const glm::vec3& value) const;
    void setMat3(const std::string &name, const glm::mat3& value) const;
    void setMat4(const std::string &name, const glm::mat4& value) const;
    void setBlock(const std::string& name, const GLuint bindingPoint) const;
};

#endif /* shader_hpp */
