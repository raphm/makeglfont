//
//  distance_map.cpp
//  makeglfont


// This code comes from Freetype GL.

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


#include <stdlib.h>
#include <vector>
#include <cmath>

#include "edtaa3func.h"
#include "distance_map.h"

// From freetype-gl.
void make_distance_map( double *data, unsigned int width, unsigned int height )
{
    short * xdist = (short *)  malloc( width * height * sizeof(short) );
    short * ydist = (short *)  malloc( width * height * sizeof(short) );
    double * gx   = (double *) calloc( width * height, sizeof(double) );
    double * gy      = (double *) calloc( width * height, sizeof(double) );
    double * outside = (double *) calloc( width * height, sizeof(double) );
    double * inside  = (double *) calloc( width * height, sizeof(double) );
    int i;
    
    // Compute outside = edtaa3(bitmap); % Transform background (0's)
    computegradient( data, height, width, gx, gy);
    edtaa3(data, gx, gy, width, height, xdist, ydist, outside);
    for( i=0; i<width*height; ++i)
    {
        if( outside[i] < 0.0 )
        {
            outside[i] = 0.0;
        }
    }
    
    // Compute inside = edtaa3(1-bitmap); % Transform foreground (1's)
    memset( gx, 0, sizeof(double)*width*height );
    memset( gy, 0, sizeof(double)*width*height );
    for( i=0; i<width*height; ++i)
        data[i] = 1 - data[i];
    computegradient( data, height, width, gx, gy );
    edtaa3( data, gx, gy, width, height, xdist, ydist, inside );
    for( i=0; i<width*height; ++i )
    {
        if( inside[i] < 0 )
        {
            inside[i] = 0.0;
        }
    }
    
    // distmap = outside - inside; % Bipolar distance field
    float vmin = +INFINITY;
    for( i=0; i<width*height; ++i)
    {
        outside[i] -= inside[i];
        if( outside[i] < vmin )
        {
            vmin = outside[i];
        }
    }
    vmin = abs(vmin);
    for( i=0; i<width*height; ++i)
    {
        float v = outside[i];
        if     ( v < -vmin) outside[i] = -vmin;
        else if( v > +vmin) outside[i] = +vmin;
        data[i] = (outside[i]+vmin)/(2*vmin);
    }
    
    free( xdist );
    free( ydist );
    free( gx );
    free( gy );
    free( outside );
    free( inside );
}

unsigned char *
make_distance_map( unsigned char *img, unsigned int width, unsigned int height )
{
    short * xdist = (short *)  malloc( width * height * sizeof(short) );
    short * ydist = (short *)  malloc( width * height * sizeof(short) );
    double * gx   = (double *) calloc( width * height, sizeof(double) );
    double * gy      = (double *) calloc( width * height, sizeof(double) );
    double * data    = (double *) calloc( width * height, sizeof(double) );
    double * outside = (double *) calloc( width * height, sizeof(double) );
    double * inside  = (double *) calloc( width * height, sizeof(double) );
    int i;
    
    // Convert img into double (data)
    double img_min = 255, img_max = -255;
    for( i=0; i<width*height; ++i)
    {
        double v = img[i];
        data[i] = v;
        if (v > img_max) img_max = v;
        if (v < img_min) img_min = v;
    }
    // Rescale image levels between 0 and 1
    for( i=0; i<width*height; ++i)
    {
        data[i] = (img[i]-img_min)/img_max;
    }
    
    // Compute outside = edtaa3(bitmap); % Transform background (0's)
    computegradient( data, height, width, gx, gy);
    edtaa3(data, gx, gy, height, width, xdist, ydist, outside);
    for( i=0; i<width*height; ++i)
        if( outside[i] < 0 )
            outside[i] = 0.0;
    
    // Compute inside = edtaa3(1-bitmap); % Transform foreground (1's)
    memset(gx, 0, sizeof(double)*width*height );
    memset(gy, 0, sizeof(double)*width*height );
    for( i=0; i<width*height; ++i)
        data[i] = 1 - data[i];
    computegradient( data, height, width, gx, gy);
    edtaa3(data, gx, gy, height, width, xdist, ydist, inside);
    for( i=0; i<width*height; ++i)
        if( inside[i] < 0 )
            inside[i] = 0.0;
    
    // distmap = outside - inside; % Bipolar distance field
    unsigned char *out = (unsigned char *) malloc( width * height * sizeof(unsigned char) );
    for( i=0; i<width*height; ++i)
    {
        outside[i] -= inside[i];
        outside[i] = 128+outside[i]*16;
        if( outside[i] < 0 ) outside[i] = 0;
        if( outside[i] > 255 ) outside[i] = 255;
        out[i] = 255 - (unsigned char) outside[i];
    }
    
    free( xdist );
    free( ydist );
    free( gx );
    free( gy );
    free( data );
    free( outside );
    free( inside );
    return out;
}

// The following functions are from freetype-gl's "demo-distance-field-3.c"

// ------------------------------------------------------ MitchellNetravali ---
// Mitchell Netravali reconstruction filter
float
MitchellNetravali( float x )
{
    const float B = 1/3.0, C = 1/3.0; // Recommended
    // const float B =   1.0, C =   0.0; // Cubic B-spline (smoother results)
    // const float B =   0.0, C = 1/2.0; // Catmull-Rom spline (sharper results)
    x = fabs(x);
    if( x < 1 )
        return ( ( 12 -  9 * B - 6 * C) * x * x * x
                + (-18 + 12 * B + 6 * C) * x * x
                + (  6 -  2 * B) ) / 6;
    else if( x < 2 )
        return ( (     -B -  6 * C) * x * x * x
                + (  6 * B + 30 * C) * x * x
                + (-12 * B - 48 * C) * x
                + (  8 * B + 24 * C) ) / 6;
    else
        return 0;
}


// ------------------------------------------------------------ interpolate ---
float
interpolate( float x, float y0, float y1, float y2, float y3 )
{
    float c0 = MitchellNetravali(x-1);
    float c1 = MitchellNetravali(x  );
    float c2 = MitchellNetravali(x+1);
    float c3 = MitchellNetravali(x+2);
    float r =  c0*y0 + c1*y1 + c2*y2 + c3*y3;
    return std::min( std::max( r, 0.0f ), 1.0f );
}


// ------------------------------------------------------------------ scale ---
int
resize( double *src_data, size_t src_width, size_t src_height,
       double *dst_data, size_t dst_width, size_t dst_height )
{
    if( (src_width == dst_width) && (src_height == dst_height) )
    {
        memcpy( dst_data, src_data, src_width*src_height*sizeof(double));
        return 0;
    }
    size_t i,j;
    float xscale = src_width / (float) dst_width;
    float yscale = src_height / (float) dst_height;
    for( j=0; j < dst_height; ++j )
    {
        for( i=0; i < dst_width; ++i )
        {
            int src_i = (int) floor( i * xscale );
            int src_j = (int) floor( j * yscale );
            int i0 = std::min( std::max( 0, src_i-1 ), (int)src_width-1 );
            int i1 = std::min( std::max( 0, src_i   ), (int)src_width-1 );
            int i2 = std::min( std::max( 0, src_i+1 ), (int)src_width-1 );
            int i3 = std::min( std::max( 0, src_i+2 ), (int)src_width-1 );
            int j0 = std::min( std::max( 0, src_j-1 ), (int)src_height-1 );
            int j1 = std::min( std::max( 0, src_j   ), (int)src_height-1 );
            int j2 = std::min( std::max( 0, src_j+1 ), (int)src_height-1 );
            int j3 = std::min( std::max( 0, src_j+2 ), (int)src_height-1 );
            float t0 = interpolate( i / (float) dst_width,
                                   src_data[j0*src_width+i0],
                                   src_data[j0*src_width+i1],
                                   src_data[j0*src_width+i2],
                                   src_data[j0*src_width+i3] );
            float t1 = interpolate( i / (float) dst_width,
                                   src_data[j1*src_width+i0],
                                   src_data[j1*src_width+i1],
                                   src_data[j1*src_width+i2],
                                   src_data[j1*src_width+i3] );
            float t2 = interpolate( i / (float) dst_width,
                                   src_data[j2*src_width+i0],
                                   src_data[j2*src_width+i1],
                                   src_data[j2*src_width+i2],
                                   src_data[j2*src_width+i3] );
            float t3 = interpolate( i / (float) dst_width,
                                   src_data[j3*src_width+i0],
                                   src_data[j3*src_width+i1],
                                   src_data[j3*src_width+i2],
                                   src_data[j3*src_width+i3] );
            float y =  interpolate( j / (float) dst_height, t0, t1, t2, t3 );
            dst_data[j*dst_width+i] = y;
        }
    }
    return 0;
}

// End of freetype-gl functions.
