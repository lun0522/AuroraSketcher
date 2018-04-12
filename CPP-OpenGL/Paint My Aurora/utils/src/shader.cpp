//
//  shader.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/9/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"

using std::string;
using std::runtime_error;

static std::unordered_map<string, string> loadedCode;

string Shader::readCode(string path) {
    auto loaded = loadedCode.find(path);
    if (loaded == loadedCode.end()) {
        using std::stringstream;
        using std::ifstream;
        
        stringstream stream;
        ifstream file(path);
        file.exceptions(ifstream::failbit | ifstream::badbit);
        if (!file.is_open()) throw runtime_error("Cannot open file: " + path);
        
        try {
            stream << file.rdbuf();
            string code = stream.str();
            loadedCode.insert({ path, code });
            return code;
        } catch (ifstream::failure e) {
            throw runtime_error("Failed in reading file: " + e.code().message());
        }
    } else {
        return loaded->second;
    }
}

GLuint createShader(GLenum type, string source) {
    // .c_str() returns a r-value, should make it l-value and let it persist
    const char *srcString = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &srcString, NULL); // can pass an array of strings
    glCompileShader(shader);
    return shader;
}

void validateShader(GLuint shader) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        throw runtime_error(infoLog);
    }
}

GLuint createProgram(GLuint vertexShader, GLuint fragmentShader, GLint geometryShader) {
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    if (geometryShader != GL_INVALID_INDEX) glAttachShader(shaderProgram, geometryShader);
    glLinkProgram(shaderProgram);
    return shaderProgram;
}

void validateLink(GLuint program) {
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        throw runtime_error(infoLog);
    }
}

Shader::Shader(const std::string& vertexPath,
               const std::string& fragmentPath,
               const std::string& geometryPath) {
    GLuint vertex = createShader(GL_VERTEX_SHADER, readCode(vertexPath).c_str());
    GLuint fragment = createShader(GL_FRAGMENT_SHADER, readCode(fragmentPath).c_str());
    GLint geometry = geometryPath.empty() ? GL_INVALID_INDEX : createShader(GL_GEOMETRY_SHADER, readCode(geometryPath).c_str());
    try {
        validateShader(vertex);
        validateShader(fragment);
        if (geometry != GL_INVALID_INDEX) validateShader(geometry);
    } catch (string log) {
        throw runtime_error("Failed in creating shader: " + log);
    }
    
    programId = createProgram(vertex, fragment, geometry);
    try {
        validateLink(programId);
    } catch (string log) {
        throw runtime_error("Failed in linking: " + log);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (!geometryPath.empty()) glDeleteShader(geometry);
}

void Shader::use() const {
    glUseProgram(programId);
}

GLuint Shader::getUniform(const string &name) const {
    GLint location = glGetUniformLocation(programId, name.c_str());
    if (location != GL_INVALID_INDEX) return location;
    else throw runtime_error("Cannot find uniform " + name);
}

void Shader::setInt(const string &name, const int value) const {
    glUniform1i(getUniform(name), value);
}

void Shader::setFloat(const string &name, const float value) const {
    glUniform1f(getUniform(name), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2& value) const {
    glUniform2fv(getUniform(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, const GLfloat v0, const GLfloat v1, const GLfloat v2) const {
    glUniform3f(getUniform(name), v0, v1, v2);
}

void Shader::setVec3(const std::string &name, const glm::vec3& value) const {
    glUniform3fv(getUniform(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(const std::string &name, const glm::mat3& value) const {
    // how many matrices to send, transpose or not (GLM is already in coloumn order, so no)
    glUniformMatrix3fv(getUniform(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniform(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setBlock(const std::string& name, const GLuint bindingPoint) const {
    GLint blockIndex = glGetUniformBlockIndex(programId, name.c_str());
    if (blockIndex == GL_INVALID_INDEX) throw runtime_error("Cannot find block " + name);
    glUniformBlockBinding(programId, blockIndex, bindingPoint);
}
