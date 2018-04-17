//
//  loader.hpp
//  LearnOpenGL
//
//  Created by Pujun Lun on 3/31/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef loader_hpp
#define loader_hpp

#include <vector>
#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Loader {
public:
    static GLuint loadTexture(const std::string& path, const bool gammaCorrection);
    static GLuint loadCubemap(const std::string& path,
                              const std::vector<std::string>& filename,
                              const bool gammaCorrection);
    static GLuint loadCharacter(const std::string& fontPath,
                                const std::string& vertPath,
                                const std::string& fragPath,
                                const std::vector<std::string>& texts,
                                std::unordered_map<char, glm::vec2>& charOffsets,
                                const GLuint prevFrameBuffer,
                                const glm::vec4 prevViewPort);
};

#endif /* loader_hpp */
