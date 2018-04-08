//
//  loader.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/31/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "loader.hpp"

using std::vector;
using std::string;

GLuint loadImage(const string& path,
                 const GLenum target,
                 const bool shouldBind,
                 const bool gammaCorrection) {
    int width, height, channel;
    stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channel, 0);
    if (!data) throw std::runtime_error("Failed to load texture from " + path);
    
    GLenum internalFormat, format;
    switch (channel) {
        case 1:
            internalFormat = format = GL_RED;
            break;
        case 3:
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            format = GL_RGB;
            break;
        case 4:
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            format = GL_RGBA;
            break;
        default:
        throw std::runtime_error("Unknown texture format (channel="
                                 + std::to_string(channel) + ")");
    }
    
    GLuint texture = 0;
    if (shouldBind) {
        glGenTextures(1, &texture);
        glBindTexture(target, texture);
    }
    
    // texture target, minmap level, texture format, width, height, always 0, image format, dtype, data
    glTexImage2D(target, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    stbi_image_free(data);
    return texture;
}

GLuint Loader::loadTexture(const string& path, const bool gammaCorrection) {
    GLuint texture = loadImage(path, GL_TEXTURE_2D, true, gammaCorrection);
    
    glGenerateMipmap(GL_TEXTURE_2D); // automatically generate all required minmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

GLuint Loader::loadCubemap(const string& path,
                           const vector<string>& filename,
                           const bool gammaCorrection) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    
    for (int i = 0; i < filename.size(); ++i) {
        string filepath = path + '/' + filename[i];
        loadImage(filepath, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, false, gammaCorrection);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return texture;
}


