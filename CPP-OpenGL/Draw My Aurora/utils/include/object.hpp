//
//  object.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/18/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef object_hpp
#define object_hpp

#include <string>

#include <glad/glad.h>

#include "shader.hpp"

class Object {
    GLuint VAO;
    unsigned long numVertices;
public:
    Object(const std::string& path);
    void draw(Shader& shader) const;
};

#endif /* object_hpp */
