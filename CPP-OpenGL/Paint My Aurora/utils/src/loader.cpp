//
//  loader.cpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/31/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <stdexcept>
#include <unordered_set>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "shader.hpp"
#include "loader.hpp"

using std::vector;
using std::string;
using std::runtime_error;
using std::unordered_map;
using std::unordered_set;
using glm::ivec2;
using glm::vec2;
using glm::vec4;

struct CharGlyph {
    GLuint texture;
    ivec2 size;
    ivec2 bearing;
    int advance;
};

static const int CHAR_HEIGHT = 64;
static unordered_map<string, GLuint> loadedTexture;

GLuint loadImage(const string& path,
                 const GLenum target,
                 const bool shouldBind,
                 const bool gammaCorrection) {
    int width, height, channel;
    stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channel, 0);
    if (!data) throw runtime_error("Failed to load texture from " + path);
    
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
            throw runtime_error("Unknown texture format (channel=" + std::to_string(channel) + ")");
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
    auto loaded = loadedTexture.find(path);
    if (loaded == loadedTexture.end()) {
        GLuint texture = loadImage(path, GL_TEXTURE_2D, true, gammaCorrection);
        
        glGenerateMipmap(GL_TEXTURE_2D); // automatically generate all required minmaps
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        loadedTexture.insert({ path, texture });
        return texture;
    } else {
        return loaded->second;
    }
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

void loadAllChars(const string& path,
                  const vector<char>& chars,
                  vector<CharGlyph>& loadedChars) {
    FT_Library lib;
    if (FT_Init_FreeType(&lib))
        throw runtime_error("Failed to init FreeType library");
    
    FT_Face face;
    if (FT_New_Face(lib, path.c_str(), 0, &face))
        throw runtime_error("Failed to load font");
    
    FT_Set_Pixel_Sizes(face, 0, CHAR_HEIGHT); // set width to 0 for auto adjustment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment for texture storage
    
    for (const char& c : chars) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            throw runtime_error("Failed to load glyph");
        
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        loadedChars.push_back({
            texture,
            ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (int) face->glyph->advance.x >> 6, // advance is number of 1/64 pixels
        });
    }
    
    FT_Done_Face(face);
    FT_Done_FreeType(lib);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

GLuint Loader::loadCharacter(const string& fontPath,
                             const string& vertPath,
                             const string& fragPath,
                             const vector<string>& texts,
                             unordered_map<char, Character>& charFrame,
                             const GLuint prevFrameBuffer,
                             const vec4 prevViewPort) {
    // ------------------------------------
    // extract all unique chars from the input text
    // and load corresponding texture from the library
    
    unordered_set<char> charsSet;
    for (const string& text : texts)
        for (const char& c : text)
            charsSet.insert(c);
    
    vector<char> uniqueChars(charsSet.begin(), charsSet.end());
    vector<CharGlyph> loadedChars;
    loadAllChars(fontPath, uniqueChars, loadedChars);
    
    
    // ------------------------------------
    // create a framebuffer with a big texture
    
    float totalWidth = 0;
    for (const auto& ch : loadedChars)
        totalWidth += ch.advance;
    
    GLuint texColorBuffer;
    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, totalWidth, CHAR_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glViewport(0, 0, totalWidth, CHAR_HEIGHT); // important!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);
    
    
    // ------------------------------------
    // render all textures of chars on the big texture
    
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    glClear(GL_COLOR_BUFFER_BIT); // important!
    
    Shader shader(vertPath, fragPath);
    shader.use();
    shader.setInt("glyph", 0);
    glActiveTexture(GL_TEXTURE0);
    
    // offsetX tracks where we are on the big texture
    // it will rise to totalWidth at last
    float offsetX = 0.0f;
    // ratio helps map coordinates to [0, 1] (previously measured in pixels)
    // coordinates will be converted to NDC ([-1, 1]) in vertex shader
    vec2 ratio = 1.0f / vec2(totalWidth, CHAR_HEIGHT);
    for (int i = 0; i < uniqueChars.size(); ++i) {
        CharGlyph& ch = loadedChars[i];
        vec2 size = vec2(ch.size) * ratio;
        // bearing is considered so that characters won't be too close
        float originX = (offsetX + ch.bearing.x) / totalWidth;
        float vertexAttrib[6][4] = {
            { originX,          size.y,  0.0, 0.0 },
            { originX,          0.0f,    0.0, 1.0 },
            { originX + size.x, 0.0f,    1.0, 1.0 },
            
            { originX,          size.y,  0.0, 0.0 },
            { originX + size.x, 0.0f,    1.0, 1.0 },
            { originX + size.x, size.y,  1.0, 0.0 },
        };
        charFrame.insert({ uniqueChars[i], { originX, size, ch.size, ch.bearing, ch.advance } });
        offsetX += ch.advance;
        
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexAttrib), vertexAttrib);
        glBindTexture(GL_TEXTURE_2D, ch.texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDeleteTextures(1, &ch.texture); // no longer needed
    }
    
    
    // ------------------------------------
    // clean up
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, prevFrameBuffer);
    glViewport(prevViewPort.x, prevViewPort.y, prevViewPort.z, prevViewPort.w);
    
    return texColorBuffer;
}
