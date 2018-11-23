//
//  loader.hpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/31/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef loader_hpp
#define loader_hpp

#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace Loader {
    /*
     originX: where the glyph of this character starts on the big texture
     (bearing has been counted)
     extent: the size of character on the big texture
     size, bearing, advance: inherited from the original glyph
     */
    struct Character {
        float originX;
        glm::vec2 extent;
        glm::ivec2 size;
        glm::ivec2 bearing;
        int advance;
    };
    void setFlipVertically(const bool shouldFlip);
    void set2DTexParameter(const GLenum wrapMode, const GLenum interpMode);
    GLuint loadTexture(const std::string& path, const bool gammaCorrection);
    GLuint loadCubemap(const std::string& path,
                       const std::vector<std::string>& filename,
                       const bool gammaCorrection);
    GLuint loadCharacter(const std::string& fontPath,
                         const std::string& vertPath,
                         const std::string& fragPath,
                         const std::vector<std::string>& texts,
                         std::unordered_map<char, Character>& charFrame,
                         const GLuint prevFrameBuffer,
                         const glm::vec4 prevViewPort);
};

#endif /* loader_hpp */
