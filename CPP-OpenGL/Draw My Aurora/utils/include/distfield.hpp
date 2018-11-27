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

namespace DistanceField {
    struct Point {
        int dx, dy;
        int distSq() const { return dx * dx + dy * dy; }
    };
    
    class Generator {
    public:
        Generator(const int width, const int height);
        void operator()(unsigned char* image);
        ~Generator();
    private:
        int imageWidth, imageHeight;
        int gridWidth, gridHeight, numPoint;
        Point* grid;
        inline Point get(const int x, const int y);
        inline void put(const int x, const int y, const Point& p);
        inline Point groupCompare(Point other, const int x, const int y,
                                  const __m256i& offsets);
        inline Point singleCompare(Point other, const int x, const int y,
                                   const int offsetx, const int offsety);
        void generateSDF();
    };
}

#endif /* distfield_hpp */
