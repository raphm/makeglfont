

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

#ifndef makeglfont_makeglfont_h
#define makeglfont_makeglfont_h

#include <map>

#include "fbitmap.h"

// See http://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html for glyph metrics description

struct glyph
{
    uint32_t charcode;
    float bbox_width, bbox_height; // width and height of bbox in pixels
    float bearing_x; // offset from current pen position to glyph's left bbox edge
    float bearing_y; // offset from baseline to top of glyph's bbox
    float advance_x; // horizontal distance to increment pen position when glyph is drawn
    fbitmap<unsigned char> bmp;
    std::map<uint32_t, float> kernings; // map of kern pairs relative to this glyph;
    // <previous character in character pair, kern value in pixels>
    float s0, t0, s1, t1; // final texture coordinates after packing.
};

#endif
