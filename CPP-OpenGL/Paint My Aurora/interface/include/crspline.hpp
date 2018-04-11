//
//  crspline.hpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/9/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef crspline_hpp
#define crspline_hpp

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

class CRSpline {
    float height, epsilon;
    GLuint pointVAO, pointVBO, curveVAO, curveVBO;
    std::vector<glm::vec3> controlPoints, curvePoints;
    void tessellate(const glm::vec3& p0,
                    const glm::vec3& p1,
                    const glm::vec3& p2,
                    const glm::vec3& p3,
                    int depth);
    void toBezierSpline(const glm::vec3& p0,
                        const glm::vec3& p1,
                        const glm::vec3& p2,
                        const glm::vec3& p3);
    void constructSpline();
public:
    CRSpline(const std::vector<glm::vec3>& ctrlPoints,
             const float height = 1.0f,
             const float epsilon = 1E-2);
    void processMouseClick(glm::vec3& pos);
    void drawControlPoints() const;
    void drawSplineCurve() const;
};

#endif /* crspline_hpp */
