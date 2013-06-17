# makeglfont

A small utility that creates a signed distance field PNG from a TrueType or 
an OpenType font file. These sorts of PNGs are typically used by an OpenGL
renderer to display scalable text.

## Signed Distance Fields

Among other uses, signed distance fields allow a simple GLSL shader to
recover a vector-like representation of a shape from a bitmap of
values. This makes them very useful for rendering fonts in OpenGL
video games. A small (e.g., 30 by 30 pixel) bitmap holding an SDF
representation of a glyph can be smoothly scaled with no pixelation
and with very little perceptible loss in quality. A full discussion of
the why and what of signed distance fields can be found on [krytharn's
YouTube presentation][kyp] or in [Valve's original paper][vop].

[kyp]: http://www.youtube.com/watch?v=CGZRHJvJYIg
[vop]: http://www.valvesoftware.com/publications/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf

## Previously Existing Tools

A similar signed distance bitmap font tool was created by *lonesock*
and [released][lft] in the gamedev.net "APIs and Tools" forum in
2008. The difference between lonesock's tool and this tool is that
this tool uses [Stefan Gustavson's new algorithm][mdm], which makes use of a
modified distance measure based on the grayscale value of an
anti-aliased edge pixel.  This offers an improvement in quality over
the use of the "fully on" and "fully off" pixels used by previous
algorithms. I used code from the FreeType-GL project as well as C code 
released by Stefan Gustavson. Licenses are reproduced in the source
code as well as below.

[lft]: http://www.gamedev.net/topic/491938-signed-distance-bitmap-font-tool/
[mdm]: http://contourtextures.wikidot.com

## Example Usage

`./glfont fontname.ttf 512`, where 512 is the bitmap size. Best if it's a power of two.

You'll get a font PNG and a JSON file. Examples of how to use these are coming soon.

## Building FreeType on OS X

The Freetype libraries are relatively easy to build on OS X.

The goal is to build an i386 version and an x86_64 version, then
combine them into a single library.

First, i386:

       ./configure --without-zlib --without-bzip2 --without-old-mac-fonts --without-fsspec --without-fsref --without-quickdraw-toolbox --without-quickdraw-carbon --without-ats CFLAGS="-arch i386"

       make

       cp objs/.libs/libfreetype.a libfreetype-i386.a

       make clean

Next, x86_64:

      ./configure --without-zlib --without-bzip2 --without-old-mac-fonts --without-fsspec --without-fsref --without-quickdraw-toolbox --without-quickdraw-carbon --without-ats CFLAGS="-arch x86_64"

      make

      cp objs/.libs/libfreetype.a libfreetype-x86_64.a

Now combine them:

    lipo -create -output libfreetype.a libfreetype-i386.a libfreetype-x86_64.a

`libfreetype.a` is the library we'll link against.

You can see the contents by doing:

    lipo -info libfreetype.a

Change directories to the makeglfont directory. Make a build directory.

When you run CMake, for the first time, you will see:

     -- Could NOT find Freetype (missing:  FREETYPE_LIBRARY FREETYPE_INCLUDE_DIRS) 
     -- Configuring done
     -- Generating done
     -- Build files have been written to: /Users/raph/src-c/makeglfont/build

Make the FREETYPE_LIBRARY and FREETYPE_INCLUDE_DIRS available to CMake:

(Obviously, you'll have to change "../../freetype-2.4.12" to point to the path where you built your copy of freetype.)

	    $ export FREETYPE_DIR=../../freetype-2.4.12/
	    $ cmake -G Xcode ../

Done.

## License and Disclaimer

This software &copy; 2010 [Raphael Martelles](http://www.raphm.com)
and is released under the following license:

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


The font `mensch.ttf` is from http://robey.lag.net/2010/06/21/mensch-font.html.

Some portions of this code come from freetype-gl:

/* =========================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * -------------------------------------------------------------------------
 * Copyright 2011 Nicolas P. Rougier. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ========================================================================= */

Some portions of this code come from Stefan Gustavson's anti-aliased Euclidean
distance transform (http://contourtextures.wikidot.com):

/*
 Copyright (C) 2009-2012 Stefan Gustavson (stefan.gustavson@gmail.com)
 The code in this file is distributed under the MIT license:
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

