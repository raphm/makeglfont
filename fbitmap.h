/* =========================================================================
 * MakeGLFont
 * Platform:    Any
 * WWW:
 * -------------------------------------------------------------------------
 * Copyright 2013 Raphael Martelles. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY RAPHAEL MARTELLES ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL RAPHAEL MARTELLES OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Raphael Martelles.
 * ========================================================================= */

#ifndef __makeglfont__fbitmap__
#define __makeglfont__fbitmap__

#include <vector>

template <typename T>
struct fbitmap
{
    int width=0, height=0;
    std::vector<T> data;
    fbitmap<T>():width(0), height(0), data() {}
    fbitmap<T>(int w, int h, T val):width(w),height(h),data() {
        data.resize(w*h);
        std::fill(data.begin(), data.end(), val);
    }
};

namespace fbmp {
    
    template <typename T>
    inline bool clear(fbitmap<T> & bmp, T val) {
        bmp.data.resize(bmp.width*bmp.height);
        std::fill(bmp.data.begin(), bmp.data.end(), val);
        return true;
    }
    
    template <typename T>
    inline int get_idx(fbitmap<T> const& bmp, int x, int y) {
        // (x,y) are coordinates from *bottom* origin (0,0) of bitmap.
        // We store the bitmap in a contiguous array, so the array 0 byte
        // is really the x=0,y=top byte of the bitmap.
        assert(x >= 0 && y >= 0 && x < bmp.width && y < bmp.height );
        int rows_from_bottom = (bmp.height-1)-y;
        int arrpos = bmp.width*rows_from_bottom;
        arrpos += x;
        return arrpos;
    }
    
    template <typename T>
    inline T get(fbitmap<T> const & bmp, const int x, const int y) {
        int idx = get_idx(bmp, x, y);
        return bmp.data[idx];
    }
    
    template <typename T>
    inline void set(fbitmap<T> & bmp, const int x, const int y, const T val) {
        int idx = get_idx(bmp, x, y);
        bmp.data[idx] = val;
    }
    
    /**
     * replace_bitmap()
     * Replaces a portion of the destination bitmap with the new bitmap,
     * starting from coordinates "left" and "bottom", which are offsets
     * from the 0,0 origin of the destination bitmap. I.e., "X" (below)
     * would be at x_left = 2, y_bottom = 2.
     *   3
     *   2  X
     *   1
     *   0
     *    0123
     */
    template <typename T>
    inline bool replace_part(fbitmap<T> & dest, fbitmap<T> const & src, int x_left, int y_bottom)
    {
        
        if((x_left+src.width)>dest.width || (y_bottom+src.height)>dest.height) {
            std::cout << "Error: New bitmap is too large to fit!" << std::endl;
            return false;
        }

        for (int row = 0; row < src.height; row+=1) {
            for (int col = 0; col < src.width ; col+=1) {
                set(dest, col+x_left, row+y_bottom, get(src, col, row));
            }
        }
        return true;
    }
}

#endif /* defined(__makeglfont__fbitmap__) */
