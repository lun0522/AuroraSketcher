//
//  distfield.hpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/23/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#ifndef distfield_hpp
#define distfield_hpp

#include <immintrin.h>

class DistanceField {
public:
    struct Point {
        int dx, dy;
        int distSq() const { return dx * dx + dy * dy; }
    };
    struct Grid {
        Point *points;
    };
    DistanceField(const int width, const int height);
    void generate(unsigned char* image);
    ~DistanceField();
private:
    int imageWidth, imageHeight;
    int gridWidth, gridHeight, numPoint;
    Grid grid;
    Point get(const int x, const int y);
    void put(const int x, const int y, const Point &p);
    Point groupCompare(Point other, const int x, const int y, const __m256i& offsets);
    Point singleCompare(Point other, const int x, const int y,
                        const int offsetx, const int offsety);
    void generateSDF();
};

#endif /* distfield_hpp */
