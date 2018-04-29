/*
 Original header:
 GLSL fragment shader: raytracer with planetary atmosphere.
 Dr. Orion Sky Lawlor, olawlor@acm.org, 2010-09-04 (Public Domain)
 */

/*
 This code is modified from Dr. Orion Sky Lawlor's implementation.
 */

//
//  airtrans.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/28/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include <math.h>
#include <glm/glm.hpp>

#include "airtrans.hpp"

using glm::vec3;

namespace AirTrans {
    /* A 3D ray shooting through space */
    struct ray {
        vec3 S, D; /* start location and direction (unit length) */
    };
    vec3 ray_at(ray r, float t) { return r.S + r.D * t; }
    
    /* A span of ray t values */
    struct span {
        float l, h; /* lowest, and highest t value */
    };
    
    struct sphere {
        vec3 center;
        float r;
    };
    
    const float km = 1.0 / 6378.1; // convert kilometers to render units (planet radii)
    const float miss_t = 100.0; // t value for a miss
    
    /* Return span of t values at intersection region of ray and sphere */
    span span_sphere(sphere s, ray r) {
        float b = 2.0 * dot(r.S - s.center, r.D);
        float c = dot(r.S - s.center, r.S - s.center) - s.r * s.r;
        float det = b * b - 4.0 * c;
        if (det < 0.0) return { miss_t, miss_t }; /* total miss */
        float sd = sqrt(det);
        float tL = (-b - sd) * 0.5;
        float tH = (-b + sd) * 0.5;
        return { tL, tH };
    }
    
    /************** Atmosphere Integral Approximation **************/
    /**
     Decent little Wikipedia/Winitzki 2003 approximation to erf.
     Supposedly accurate to within 0.035% relative error.
     */
    const float a = 8.0 * (M_PI - 3.0) / (3.0 * M_PI * (4.0 - M_PI));
    float erf_guts(float x) {
        float x2 = x * x;
        return exp(-x2 * (4.0 / M_PI + a * x2) / (1.0 + a * x2));
    }
    // "error function": integral of exp(-x*x)
    float win_erf(float x) {
        float sign = 1.0f;
        if (x < 0.0f) sign = -1.0f;
        return sign * sqrt(1.0 - erf_guts(x));
    }
    // erfc = 1.0-erf, but with less roundoff
    float win_erfc(float x) {
        if (x > 3.0) { ///< hits zero sig. digits around x==3.9
            // x is big -> erf(x) is very close to +1.0
            // erfc(x)=1-erf(x)=1-sqrt(1-e)=approx +e/2
            return 0.5 * erf_guts(x);
        } else {
            return 1.0 - win_erf(x);
        }
    }
    
    /**
     Compute the atmosphere's integrated thickness along this ray.
     The planet is assumed to be centered at origin, with unit radius.
     This is an exponential approximation:
     */
    float atmosphere_thickness(vec3 start, vec3 dir, float tstart, float tend) {
        float scaleheight = 8.0 * km; /* "scale height," where atmosphere reaches 1/e thickness (planetary radius units) */
        float k = 1.0 / scaleheight; /* atmosphere density = refDen*exp(-(height-refHt)*k) */
        float refHt = 1.0; /* height where density==refDen */
        float refDen = 100.0; /* atmosphere opacity per planetary radius */
        /* density=refDen*exp(-(height-refHt)*k) */
        float norm = sqrt(M_PI) / 2.0; /* normalization constant */
        
        // Step 1: planarize problem from 3D to 2D
        // integral is along ray: tstart to tend along start + t*dir
        float a = dot(dir, dir), b = 2.0 * dot(dir, start),c = dot(start, start);
        float tc = -b / (2.0 * a); //t value at ray/origin closest approach
        float y = sqrt(tc * tc * a + tc * b + c);
        float xL = tstart - tc;
        float xR = tend - tc;
        // integral is along line: from xL to xR at given y
        // x==0 is point of closest approach
        
        // Step 2: Find first matching radius r1-- smallest used radius
        float ySqr = y * y, xLSqr = xL * xL, xRSqr = xR * xR;
        float r1Sqr, r1;
        float isCross = 0.0;
        if (xL * xR < 0.0) { //Span crosses origin-- use radius of closest approach
            r1Sqr = ySqr;
            r1 = y;
            isCross = 1.0;
        } else { //Use either left or right endpoint-- whichever is closer to surface
            r1Sqr = xLSqr + ySqr;
            if (r1Sqr > xRSqr + ySqr) r1Sqr = xRSqr + ySqr;
            r1 = sqrt(r1Sqr);
        }
        
        // Step 3: Find second matching radius r2
        float del = 2.0 / k; //This distance is 80% of atmosphere (at any height)
        float r2 = r1 + del;
        float r2Sqr = r2 * r2;
        
        // Step 4: Find parameters for parabolic approximation to true hyperbolic distance
        // r(x)=sqrt(y^2+x^2), r'(x)=A+Cx^2; r1=r1', r2=r2'
        float x1Sqr = r1Sqr - ySqr; // rSqr = xSqr + ySqr, so xSqr = rSqr - ySqr
        float x2Sqr = r2Sqr - ySqr;
        
        float C = (r1 - r2) / (x1Sqr - x2Sqr);
        float A = r1 - x1Sqr * C - refHt;
        
        // Step 5: Compute the integral of exp(-k*(A+Cx^2)) from x==xL to x==xR
        float sqrtKC = sqrt(k * C); // variable change: z=sqrt(k*C)*x; exp(-z^2)
        float erfDel;
        if (isCross > 0.0) { //xL and xR have opposite signs-- use erf normally
            erfDel = win_erf(sqrtKC * xR) - win_erf(sqrtKC * xL);
        } else { //xL and xR have same sign-- flip to positive half and use erfc
            if (xL < 0.0) { xL = -xL; xR = -xR; }
            erfDel = win_erfc(sqrtKC * xR) - win_erfc(sqrtKC * xL);
        }
        if (abs(erfDel) > 1.0e-10) { /* parabolic approximation has acceptable roundoff */
            float eScl = exp(-k * A); // from constant term of integral
            return refDen * norm * eScl / sqrtKC * abs(erfDel);
        } else { /* erfDel==0.0 -> Roundoff!  Switch to a linear approximation:
                  a.) Create linear approximation r(x) = M*x+B
                  b.) integrate exp(-k*(M*x+B-1.0)) dx, from xL to xR
                  integral = (1.0/(-k*M)) * exp(-k*(M*x+B-1.0))
                  */
            float x1 = sqrt(x1Sqr), x2 = sqrt(x2Sqr);
            float M = (r2 - r1) / (x2 - x1); /* linear fit at (x1,r1) and (x2,r2) */
            float B = r1 - M * x1 - 1.0;
            
            float t1 = exp(-k * (M * xL + B));
            float t2 = exp(-k * (M * xR + B));
            return abs(refDen * (t2 - t1) / (k * M));
        }
    }
    
    void generate(unsigned char *image, const float sampleStep) {
        int ptr = 0;
        for (float sinVal = 0.0f; sinVal <= 1.0f; sinVal += sampleStep) {
            float angle = asin(sinVal);
            float x = cos(angle), z = sin(angle);
            ray r = { vec3(0.0f, 0.0f, 1.0f), vec3(x, 0.0f, z) };
            span airSpan = span_sphere({ vec3(0.0f), 75.0f * km + 1.0f }, r);
            float airMass = atmosphere_thickness(r.S, r.D, 0.0f, airSpan.h);
            float airTransmit = exp(-airMass) * 200.0f;
            image[ptr++] = (int)round(airTransmit);
        }
    }
}
