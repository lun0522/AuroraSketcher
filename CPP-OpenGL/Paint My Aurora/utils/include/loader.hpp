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
#include <glad/glad.h>

enum TextureType { DIFFUSE, SPECULAR, REFLECTION };

struct Texture {
    GLuint id;
    TextureType type;
    std::string path;
};

class Loader {
public:
    static GLuint loadTexture(const std::string& path, const bool gammaCorrection);
    static GLuint loadCubemap(const std::string& path,
                              const std::vector<std::string>& filename,
                              const bool gammaCorrection);
};

#endif /* loader_hpp */
