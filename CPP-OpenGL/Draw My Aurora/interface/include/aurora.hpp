//
//  aurora.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/25/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef aurora_hpp
#define aurora_hpp

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "crspline.hpp"
#include "distfield.hpp"

class Aurora {
    Shader pathShader;
    DistanceField field;
    unsigned char* image;
    GLuint pathTex, framebuffer;
public:
    Aurora(const GLuint prevFrameBuffer);
    void mainLoop(const std::vector<CRSpline>& splines,
                  const GLuint prevFrameBuffer,
                  const glm::vec4 prevViewPort);
    ~Aurora();
};

#endif /* aurora_hpp */
