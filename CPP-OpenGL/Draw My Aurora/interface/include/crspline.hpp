//
//  crspline.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/9/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef crspline_hpp
#define crspline_hpp

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "shader.hpp"

class CRSpline {
    int selected;
    float height, epsilon;
    glm::vec3 color;
    GLuint pointVAO, pointVBO, curveVAO, curveVBO;
    std::vector<glm::vec3> controlPoints, curvePoints;
    std::vector<glm::vec2> controlPointsNDC;
    const Shader &pointShader, &curveShader;
    void tessellate(const glm::vec3& p0,
                    const glm::vec3& p1,
                    const glm::vec3& p2,
                    const glm::vec3& p3,
                    int depth);
    void constructSpline();
public:
    CRSpline(const Shader& pointShader,
             const Shader& curveShader,
             const glm::vec3& color,
             const std::vector<glm::vec3>& ctrlPoints,
             const float height = 1.0f,
             const float epsilon = 1E-2);
    void deselectControlPoint();
    void processMouseClick(const bool isLeft,
                           const glm::vec3& posObject,
                           const glm::vec2& posNDC,
                           const glm::vec2& sideLengthNDC,
                           const glm::mat4& objectToNDC);
    void draw() const;
    void drawLine(const Shader& shader) const;
};

#endif /* crspline_hpp */
