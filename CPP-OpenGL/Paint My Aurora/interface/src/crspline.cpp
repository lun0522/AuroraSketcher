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
using glm::vec2;
using glm::bvec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

static const int MIN_NUM_CONTROL_POINTS = 3;
static const int MAX_NUM_CONTROL_POINTS = 100;
static const int MAX_RECURSSION_DEPTH = 10;
static const int MAX_NUM_CURVE_POINTS = pow(2, MAX_RECURSSION_DEPTH - 1);
static const int CONTROL_POINT_NOT_SELECTED = -1;

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
        curvePoints.push_back(glm::normalize(p0));
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

void CRSpline::toBezierSpline(const vec3& p0,
                              const vec3& p1,
                              const vec3& p2,
                              const vec3& p3) {
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

CRSpline::CRSpline(const std::vector<vec3>& ctrlPoints,
                   const float height,
                   const float epsilon):
controlPoints(ctrlPoints), height(height), epsilon(epsilon), selected(CONTROL_POINT_NOT_SELECTED) {
    if (controlPoints.size() < MIN_NUM_CONTROL_POINTS)
        throw runtime_error("No enough control points");
    
    auto configure = [] (GLuint& VAO, GLuint& VBO, vector<vec3>& dataSource, size_t maxLength) {
        dataSource.reserve(maxLength);
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, maxLength * sizeof(vec3), dataSource.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
        glEnableVertexAttribArray(0);
    };
    
    constructSpline();
    configure(pointVAO, pointVBO, controlPoints, MAX_NUM_CONTROL_POINTS);
    configure(curveVAO, curveVBO, curvePoints, MAX_NUM_CONTROL_POINTS * MAX_NUM_CURVE_POINTS);
}

void CRSpline::deselectControlPoint() {
    selected = CONTROL_POINT_NOT_SELECTED;
}

void CRSpline::processMouseClick(const bool isLeft,
                                 const vec3& posObject,
                                 const vec2& posNDC,
                                 const vec2& sideLengthNDC,
                                 const mat4& objectToNDC) {
    // note that the height of control points should be considered in all cases
    // the click is on the atmosphere, rather than the surface of earth
    auto getCoordInNDC = [&] (vec3& point) -> vec2 {
        vec4 coordInNDC = objectToNDC * vec4(point * height, 1.0f);
        return vec2(coordInNDC) / coordInNDC.w;
    };
    
    auto findClosestControlPoint = [&] (int& candidate) -> int {
        // step 1: find the control point closest to the click point
        candidate = CONTROL_POINT_NOT_SELECTED;
        float minDist = FLT_MAX;
        for (int i = 0; i < controlPoints.size(); ++i) {
            float dist = glm::distance(posObject, controlPoints[i] * height);
            if (dist < minDist) {
                minDist = dist;
                candidate = i;
            }
        }
        
        // step 2: check whether they are close enough in NDC
        vec2 diff = posNDC - getCoordInNDC(controlPoints[candidate]);
        bvec2 isClose = glm::lessThanEqual(glm::abs(diff), sideLengthNDC * 0.5f);
        return glm::all(isClose) ? candidate : CONTROL_POINT_NOT_SELECTED;
    };
    
    auto recalculatePoints = [&] () {
        auto updateData = [] (GLuint VBO, vector<vec3>& dataSource) {
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, dataSource.size() * sizeof(vec3), dataSource.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        };
        curvePoints.clear();
        constructSpline();
        updateData(pointVBO, controlPoints);
        updateData(curveVBO, curvePoints);
    };
    
    if (isLeft) {
        if (selected == CONTROL_POINT_NOT_SELECTED) {
            int candidate;
            selected = findClosestControlPoint(candidate);
        } else {
            controlPoints[selected] = posObject / height;
            recalculatePoints();
        }
    } else {
        int candidate;
        int closestPoint = findClosestControlPoint(candidate);
        if (closestPoint != CONTROL_POINT_NOT_SELECTED) { // delete the closest control point
            controlPoints.erase(controlPoints.begin() + closestPoint);
        } else { // add a point between the closest and the second closest control point
            int prevPoint = (candidate - 1) % controlPoints.size();
            int nextPoint = (candidate + 1) % controlPoints.size();
            float prevDist = glm::distance(posNDC, getCoordInNDC(controlPoints[prevPoint]));
            float nextDist = glm::distance(posNDC, getCoordInNDC(controlPoints[nextPoint]));
            int insertIndex = prevDist < nextDist ? candidate : nextPoint;
            controlPoints.insert(controlPoints.begin() + insertIndex, posObject / height);
        }
        recalculatePoints();
        selected = CONTROL_POINT_NOT_SELECTED;
    }
}

void CRSpline::drawControlPoints() const {
    glBindVertexArray(pointVAO);
    glDrawArrays(GL_POINTS, 0, controlPoints.size());
}

void CRSpline::drawSplineCurve() const {
    glBindVertexArray(curveVAO);
    glDrawArrays(GL_LINE_STRIP, 0, curvePoints.size());
}
