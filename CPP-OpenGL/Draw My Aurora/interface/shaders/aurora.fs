/*
 Otiginal header:
 GLSL fragment shader: raytracer with planetary atmosphere.
 Dr. Orion Sky Lawlor, olawlor@acm.org, 2010-09-04 (Public Domain)
 */

/*
 This shader code is modified from Dr. Orion Sky Lawlor's implementation.
 */

#version 330 core

in vec3 position;

out vec4 fragColor;

uniform vec3 cameraPos; // camera position, world coordinates
uniform sampler2D auroraDeposition; // deposition function
uniform sampler2D auroraTexture; // actual curtains and color
uniform sampler2D distanceField; // distance from curtains (0==far away, 1==close)

const float M_PI = 3.1415926535;
const float km = 1.0 / 6371.0; // convert kilometers to render units (planet radii)
const float miss_t = 100.0; // t value for a miss
const float min_t = 0.000001; // minimum acceptable t value
const float dt = 2.0 * km; // sampling rate for aurora: fine sampling gets *SLOW*
const float auroraScale = dt / (30.0 * km); // scale factor: samples at dt -> screen color
const vec3 airColor = 0.05 * vec3(0.4, 0.5, 0.7);

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

/* Return t value at first intersection of ray and sphere */
float intersect_sphere(sphere s, ray r) {
    float b = 2.0 * dot(r.S - s.center, r.D);
    float c = dot(r.S - s.center, r.S - s.center) - s.r * s.r;
    float det = b * b - 4.0 * c;
    if (det < 0.0) return miss_t; /* total miss */
    float t = (-b - sqrt(det)) * 0.5;
    if (t < min_t) return miss_t; /* behind head: miss! */
    return t;
}

/* Return span of t values at intersection region of ray and sphere */
span span_sphere(sphere s, ray r) {
    float b = 2.0 * dot(r.S - s.center, r.D);
    float c = dot(r.S - s.center, r.S - s.center) - s.r * s.r;
    float det = b * b - 4.0 * c;
    if (det < 0.0) return span(miss_t, miss_t); /* total miss */
    float sd = sqrt(det);
    float tL = (-b - sd) * 0.5;
    float tH = (-b + sd) * 0.5;
    return span(tL, tH);
}

/* Return the amount of auroral energy deposited at this height,
 measured in planetary radii. */
vec3 deposition_function(float height) {
    height -= 1.0; /* convert to altitude (subtract off planet's radius) */
    float maxHeight = 300.0 * km;
    vec3 tex = vec3(texture(auroraDeposition, vec2(0.4, 1.0 - height / maxHeight)));
    return tex * tex; // HDR storage
}

// Apply nonlinear tone mapping to final summed output color
vec3 tone_map(vec3 color) {
    float len = length(color);
    return color * pow(len, 1.0 / 2.2 - 1.0); /* convert to sRGB: scale by len^(1/2.2)/len */
}

/* Convert a 3D location to a 2D aurora map index (polar stereographic) */
vec2 down_to_map(vec3 worldPos) {
    vec3 onPlanet = normalize(worldPos);
    vec2 mapCoords = vec2(onPlanet) * 0.5 + vec2(0.5); // on 2D polar stereo map
    return mapCoords;
}

/* Sample the aurora's color at this 3D point */
vec3 sample_aurora(vec3 loc) {
    /* project sample point to surface of planet, and look up in texture */
    float r = length(loc);
    vec3 deposition = deposition_function(r);
    vec3 curtain = vec3(texture(auroraTexture, down_to_map(loc)).g);
    return deposition * curtain;
}

/* Sample the aurora's color along this ray, and return the summed color */
vec3 sample_aurora(ray r, span s) {
    if (s.h < 0.0) return vec3(0.0); /* whole span is behind our head */
    if (s.l < 0.0) s.l = 0.0; /* start sampling at observer's head */
    
    /* Sum up aurora light along ray span */
    vec3 sum = vec3(0.0);
    vec3 loc = ray_at(r, s.l); /* start point along ray */
    float t = s.l;
    while (t < s.h) {
        vec3 loc = ray_at(r, t);
        sum += sample_aurora(loc); // real curtains
        float dist = (0.99 - texture(distanceField, down_to_map(loc)).r) * 0.2;
        if (dist < dt) dist = dt;
        t += dist;
    }
    
    return sum * auroraScale; // full curtain
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
    return sign(x) * sqrt(1.0 - erf_guts(x));
}
// erfc = 1.0-erf, but with less roundoff
float win_erfc(float x) {
    if (x > 3.0) { //<- hits zero sig. digits around x==3.9
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

void main(void) {
    // Start with a camera ray
    ray r = ray(cameraPos, normalize(vec3(position, 1.0) - cameraPos)); // points from camera toward proxy
    
    // Compute intersection with planet itself
    float planet_t = intersect_sphere(sphere(vec3(0.0), 1.0), r);
    if (planet_t < miss_t) { // hit planet, simply return black
        fragColor = vec4(vec3(0.0), 1.0);
        return;
    }
    
    // All our geometry
    span auroraL = span_sphere(sphere(vec3(0.0), 85.0 * km + 1.0), r);
    span auroraH = span_sphere(sphere(vec3(0.0), 300.0 * km + 1.0), r);
    
    // Atmosphere
    span airSpan = span_sphere(sphere(vec3(0.0), 75.0 * km + 1.0), r);
    float airMass = atmosphere_thickness(r.S, r.D, airSpan.l, airSpan.h);
    float airTransmit = exp(-airMass); // fraction of light penetrating atmosphere
    float airInscatter = 1.0 - airTransmit; // fraction added by atmosphere
    
    // typical planet-hitting ray:
    // H.l  (aurora)  L.l    (air+planet)
    vec3 aurora = sample_aurora(r, span(auroraH.l, auroraL.l)); /* post atmosphere */
    
    vec3 total = airTransmit * aurora + airInscatter * airColor;
    
    // Must delay tone mapping until the very end, so we can sum pre and post atmosphere parts...
    fragColor = vec4(tone_map(total), 1.0);
}
