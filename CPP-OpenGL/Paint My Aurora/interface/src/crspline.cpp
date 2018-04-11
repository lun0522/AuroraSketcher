//
//  crspline.cpp
//  Paint My Aurora
//
//  Created by Pujun Lun on 4/9/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <stdexcept>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>

#include "crspline.hpp"

using std::vector;
using std::runtime_error;
using glm::vec3;
using glm::vec4;
using glm::mat4;

static const int MIN_NUM_CONTROL_POINTS = 3;
static const int MAX_RECURSSION_DEPTH = 10;

// following matrices are transposed
static const mat4 BEZIER_COEFF_T(-1.0f,  3.0f, -3.0f,  1.0f,
                                 3.0f, -6.0f,  3.0f,  0.0f,
                                -3.0f,  3.0f,  0.0f,  0.0f,
                                 1.0f,  0.0f,  0.0f,  0.0f);
static const mat4 CATMULL_ROM_COEFF_T(-0.5f,  1.5f, -1.5f,  0.5f,
                                       1.0f, -2.5f,  2.0f, -0.5f,
                                      -0.5f,  0.0f,  0.5f,  0.0f,
                                       0.0f,  1.0f,  0.0f,  0.0f);
static const mat4 CATMULL_ROM_TO_BEZIER_T = CATMULL_ROM_COEFF_T * glm::inverse(BEZIER_COEFF_T);

void CRSpline::tessellate(const vec3& p0,
                          const vec3& p1,
                          const vec3& p2,
                          const vec3& p3,
                          int depth) {
    auto isSmooth = [=] (const vec3& l0, const vec3& l1, const vec3& l2, const vec3& l3) -> bool {
        vec3 l0l1 = glm::normalize(l0 - l1);
        vec3 l1l2 = glm::normalize(l1 - l2);
        vec3 l2l3 = glm::normalize(l2 - l3);
        return glm::angle(l0l1, l1l2) < epsilon && glm::angle(l1l2, l2l3) < epsilon;
    };
    
    if (++depth == MAX_RECURSSION_DEPTH || glm::distance(p0, p3) < 1E-2 || isSmooth(p0, p1, p2, p3)) {
        curvePoints.push_back(glm::normalize(p0) * height);
    } else {
        // modified from glm::slerp
        auto middlePoint = [] (const vec3& l0, const vec3& l1) -> vec3 {
            float cosAlpha = dot(glm::normalize(l0), glm::normalize(l1));
            float alpha = acos(cosAlpha);
            float t = sin(0.5f * alpha) / sin(alpha);
            return (l0 + l1) * t;
        };
        
        vec3 p10 = middlePoint(p0, p1);
        vec3 p11 = middlePoint(p1, p2);
        vec3 p12 = middlePoint(p2, p3);
        vec3 p20 = middlePoint(p10, p11);
        vec3 p21 = middlePoint(p11, p12);
        vec3 p30 = middlePoint(p20, p21);
        tessellate(p0, p10, p20, p30, depth);
        tessellate(p30, p21, p12, p3, depth);
    }
}

void CRSpline::toBezierSpline(const glm::vec3& p0,
                              const glm::vec3& p1,
                              const glm::vec3& p2,
                              const glm::vec3& p3) {
    mat4 catmulRomPoints(vec4(p0, 0.0f), vec4(p1, 0.0f), vec4(p2, 0.0f), vec4(p3, 0.0f));
    mat4 bezierPoints = catmulRomPoints * CATMULL_ROM_TO_BEZIER_T;
    tessellate(bezierPoints[0], bezierPoints[1], bezierPoints[2], bezierPoints[3], 0);
}

void CRSpline::constructSpline() {
    for (int i = 0; i < controlPoints.size() - MIN_NUM_CONTROL_POINTS; ++i)
        toBezierSpline(controlPoints[i], controlPoints[i+1], controlPoints[i+2], controlPoints[i+3]);
    for (int i = (int)controlPoints.size() - MIN_NUM_CONTROL_POINTS; i < controlPoints.size(); ++i)
        toBezierSpline(controlPoints[(i+0) % controlPoints.size()],
                       controlPoints[(i+1) % controlPoints.size()],
                       controlPoints[(i+2) % controlPoints.size()],
                       controlPoints[(i+3) % controlPoints.size()]);
    curvePoints.push_back(curvePoints[0]); // close the curve
}

CRSpline::CRSpline(const std::vector<glm::vec3>& ctrlPoints,
                   const float height,
                   const float epsilon):
controlPoints(ctrlPoints), height(height), epsilon(epsilon) {
    if (controlPoints.size() < MIN_NUM_CONTROL_POINTS)
        throw runtime_error("No enough control points");
    
    auto configure = [] (GLuint& VAO, GLuint& VBO, vector<vec3>& dataSource) {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, dataSource.size() * sizeof(vec3), dataSource.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
        glEnableVertexAttribArray(0);
    };
    
    constructSpline();
    std::for_each(controlPoints.begin(), controlPoints.end(),
                  [=] (vec3& point) { point *= height; });
    configure(pointVAO, pointVBO, controlPoints);
    configure(curveVAO, curveVBO, curvePoints);
}

void CRSpline::processMouseClick(glm::vec3& pos) {
    
}

void CRSpline::drawControlPoints() const {
    glBindVertexArray(pointVAO);
    glDrawArrays(GL_POINTS, 0, controlPoints.size());
}

void CRSpline::drawSplineCurve() const {
    glBindVertexArray(curveVAO);
    glDrawArrays(GL_LINE_STRIP, 0, curvePoints.size());
}
