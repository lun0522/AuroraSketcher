//
//  aurora.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/25/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <image/stb_image_write.h>

#include "aurora.hpp"

using std::vector;
using glm::vec4;
using uchar = unsigned char;

static const int DISTANCE_FIELD_SIZE = 800;

Aurora::Aurora(const GLuint prevFrameBuffer):
pathShader("path.vs", "path.fs"),
field(DistanceField(DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE)) {
    image = (uchar *)malloc(DISTANCE_FIELD_SIZE * DISTANCE_FIELD_SIZE * sizeof(uchar));
    
    glGenTextures(1, &pathTex);
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Aurora::mainLoop(const vector<CRSpline>& splines,
                      const GLuint prevFrameBuffer,
                      const vec4 prevViewPort) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // draw all paths to framebuffer
    glLineWidth(10.0f);
    std::for_each(splines.begin(), splines.end(),
                  [&] (const CRSpline& spline) { spline.drawLine(pathShader); });
    glLineWidth(1.0f);
    
    // copy data to image
    glBindTexture(GL_TEXTURE_2D, pathTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // calculate distance field
    field.generate(image);
    stbi_write_jpg("path.jpg", DISTANCE_FIELD_SIZE, DISTANCE_FIELD_SIZE, 1, image, 100);
    
    glBindFramebuffer(GL_FRAMEBUFFER, prevFrameBuffer);
    glViewport(prevViewPort.x, prevViewPort.y, prevViewPort.z, prevViewPort.w);
}

Aurora::~Aurora() {
    if (image) free(image);
}
