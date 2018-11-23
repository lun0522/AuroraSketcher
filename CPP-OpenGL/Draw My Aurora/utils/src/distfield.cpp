//
//  distfield.cpp
//  Draw My Aurora
//
//  Created by Pujun Lun on 4/23/18.
//  Copyright © 2018 Pujun Lun. All rights reserved.
//

#include "distfield.hpp"

#include <math.h>

namespace DistanceField {
    static const Point inside  {     0,     0 };
    static const Point outside { 16384, 16384 };
    
    Generator::Generator(const int width, const int height):
    imageWidth(width), imageHeight(height),
    gridWidth(width + 2), gridHeight(height + 2) {
        numPoint = gridWidth * gridHeight; // include padding
        grid = (Point *)malloc(numPoint * sizeof(Point));
    }
    
    void Generator::operator()(unsigned char* image) {
        // initialize distance field
        for (int y = 0; y < imageHeight; ++y) {
            for (int x = 0; x < imageWidth; ++x) {
                if (image[y * imageWidth + x] < 128) {
                    put(x, y, outside);
                } else {
                    put(x, y, inside);
                }
            }
        }
        for (int x = 0; x < imageWidth; ++x) { // top and buttom padding
            put(x, -1, get(x, 0));
            put(x, imageHeight, get(x, imageHeight - 1));
        }
        for (int y = -1; y <= imageHeight; ++y) { // left and right padding
            put(-1, y, get(0, y));
            put(imageWidth, y, get(imageWidth - 1, y));
        }
        
        // calculate
        generateSDF();
        
        // write data back to image
        for(int y = 0; y < imageHeight; ++y) {
            for (int x = 0 ; x < imageWidth; ++x) {
                int dist = (int)(sqrt((double)get(x, y).distSq()));
                int c = dist;
                if (c < 0) c = 0;
                else if (c > 255) c = 255;
                image[y * imageWidth + x] = 255 - c;
            }
        }
    }
    
    Generator::~Generator() {
        free(grid);
    }
    
    inline Point Generator::get(const int x, const int y) {
        return grid[(y + 1) * gridWidth + (x + 1)];
    }
    
    inline void Generator::put(const int x, const int y, const Point &p) {
        grid[(y + 1) * gridWidth + (x + 1)] = p;
    }
    
    inline Point Generator::groupCompare(Point other, const int x, const int y,
                                         const __m256i& offsets) {
        Point self = get(x, y);
        
        /* Point other = Get( g, x+offsetx, y+offsety ); */
        int *offsetsPtr = (int *)&offsets;
        Point pn[4] = {
            other,
            get(x + offsetsPtr[1], y + offsetsPtr[5]),
            get(x + offsetsPtr[2], y + offsetsPtr[6]),
            get(x + offsetsPtr[3], y + offsetsPtr[7]),
        };
        
        /* other.dx += offsetx; other.dy += offsety; */
        __m256i *pnPtr = (__m256i *)pn;
        // x0, y0, x1, y1, x2, y2, x3, y3 -> x0, x1, x2, x3, y0, y1, y2, y3
        static const __m256i mask = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
        __m256i vecCoords = _mm256_permutevar8x32_epi32(*pnPtr, mask);
        vecCoords = _mm256_add_epi32(vecCoords, offsets);
        
        /* other.DistSq() */
        int *coordsPtr = (int *)&vecCoords;
        // note that _mm256_mul_epi32 only applies on the lower 128 bits
        __m256i vecPermuted = _mm256_permute2x128_si256(vecCoords , vecCoords, 1);
        __m256i vecSqrDists = _mm256_add_epi64(_mm256_mul_epi32(vecCoords, vecCoords),
                                               _mm256_mul_epi32(vecPermuted, vecPermuted));
        
        /* if (other.DistSq() < p.DistSq()) p = other; */
        int64_t prevDist = self.distSq(), index = -1;
        for (int i = 0; i < 4; ++i) {
            int64_t dist = *((int64_t *)&vecSqrDists + i);
            if (dist < prevDist) {
                prevDist = dist;
                index = i;
            }
        }
        if (index != -1) {
            other = { coordsPtr[index], coordsPtr[index + 4] };
            put(x, y, other);
            return other;
        } else {
            return self;
        }
    }
    
    inline Point Generator::singleCompare(Point other, const int x, const int y,
                                          const int offsetx, const int offsety) {
        Point self = get(x, y);
        other.dx += offsetx;
        other.dy += offsety;
        
        if (other.distSq() < self.distSq()) {
            put(x, y, other);
            return other;
        } else {
            return self;
        }
    }
    
    void Generator::generateSDF() {
        // Pass 0
        static const __m256i offsets0 = _mm256_setr_epi32(-1, -1, 0, 1, 0, -1, -1, -1);
        for (int y = 0; y < imageHeight; ++y) {
            Point prev = get(-1, y);
            for (int x = 0; x < imageWidth; ++x)
                prev = groupCompare(prev, x, y, offsets0);
            
            prev = get(imageWidth, y);
            for (int x = imageWidth - 1; x >= 0; --x)
                prev = singleCompare(prev, x, y, 1, 0);
        }
        
        // Pass 1
        static const __m256i offsets1 = _mm256_setr_epi32(1, -1, 0, 1, 0, 1, 1, 1);
        for (int y = imageHeight - 1; y >= 0; --y) {
            Point prev = get(imageWidth, y);
            for (int x = imageWidth - 1; x >= 0; --x)
                prev = groupCompare(prev, x, y, offsets1);
            
            prev = get(-1, y);
            for (int x = 0; x < imageWidth; ++x)
                prev = singleCompare(prev, x, y, -1, 0);
        }
    }
}
