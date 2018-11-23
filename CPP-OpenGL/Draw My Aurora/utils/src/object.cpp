//
//  object.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/18/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include "object.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

using namespace std;
using namespace glm;

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
};

static const int OBJ_FILE_INDEX_BASE = 1;

Object::Object(const string& path) {
    ifstream file(path);
    if (!file.is_open()) throw runtime_error("Cannot open file: " + path);
    
    auto split = [] (const string& s, char delimiter) -> vector<string> {
        istringstream stream(s);
        vector<string> segments;
        string segment;
        while (getline(stream, segment, delimiter))
            segments.push_back(segment);
        return segments;
    };
    
    auto startsWith = [] (const string& s, const string& t) -> bool {
        if (s.size() < t.size()) return false;
        else return s.compare(0, t.size(), t) == 0;
    };
    
    auto isComment = [] (const string& s) -> bool {
        return s.size() > 0 && s[0] == '#';
    };
    
    auto validate = [&] (const vector<string>& segments, const int count) {
        if (segments.size() < count + 1)
            throw string("too few elements");
        if (segments.size() > count + 1 && !isComment(segments[count + 1]))
            throw string("too many elements");
    };
    
    auto getNumber = [&] (const vector<string>& segments, const int count) -> vec3 {
        validate(segments, count);
        vec3 ret(0.0f);
        for (int i = 0; i < count; ++i)
            ret[i] = stof(segments[i+1]);
        return ret;
    };
    
    vector<vec3> positions;
    vector<vec2> texCoords;
    vector<vec3> normals;
    vector<Vertex> vertices;
    vector<GLuint> indices;
    unordered_map<string, GLuint> loadedVertices;
    
    for (string line; getline(file, line); ) {
        try {
            vector<string> segments = split(line, ' ');
            if (segments.size() > 0) {
                if (isComment(segments[0])) { // comment
                    continue;
                } else if (startsWith(segments[0], "f")) { // face
                    validate(segments, 3);
                    for (int i = 1; i <= 3; ++i) {
                        auto loaded = loadedVertices.find(segments[i]);
                        if (loaded != loadedVertices.end()) {
                            indices.push_back(loaded->second);
                        } else {
                            vector<string> vertIndices = split(segments[i], '/');
                            if (vertIndices.size() != 3)
                                throw string("wrong number of indices");
                            
                            GLuint posIdx = stoi(vertIndices[0]) - OBJ_FILE_INDEX_BASE;
                            GLuint texIdx = stoi(vertIndices[1]) - OBJ_FILE_INDEX_BASE;
                            GLuint normIdx = stoi(vertIndices[2]) - OBJ_FILE_INDEX_BASE;
                            vertices.push_back({
                                // use .at() to trigger out_of_range exception
                                positions.at(posIdx),
                                normals.at(normIdx),
                                texCoords.at(texIdx),
                            });
                            GLuint index = (GLuint) vertices.size() - 1;
                            indices.push_back(index);
                            loadedVertices.insert({ segments[i], index });
                        }
                    }
                } else if (startsWith(segments[0], "vt")) { // texCoord
                    texCoords.push_back(getNumber(segments, 2));
                } else if (startsWith(segments[0], "vn")) { // normal
                    normals.push_back(getNumber(segments, 3));
                } else if (startsWith(segments[0], "v")) { // position
                    positions.push_back(getNumber(segments, 3));
                } else {
                    throw string("unknown symbol");
                }
            }
        } catch (const string& err) {
            throw runtime_error("Failed to parse line: " + err + " in " + line);
        } catch (const invalid_argument& err) {
            throw runtime_error("Invalid argument: " + line);
        } catch (const out_of_range& err) {
            throw runtime_error("Index out of range: " + line);
        }
    }
    numVertices = indices.size(); // reserved for drawing. Not vertices.size() here!
    
    // VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // VBO
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // EBO
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // configure vertex attributes
    GLsizei stride = 8 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    
    // clean up
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Object::draw(Shader& shader) const {
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
