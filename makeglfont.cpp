
// makeglfont.cpp

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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };
const struct {
    int          code;
    const char*  message;
} FT_Errors[] =
#include FT_ERRORS_H

// Nemanja Trifunovic's UTF-8 translation headers.
#include "utf8.h"

// Sean Barrett's STB Image Write, to save our bitmaps.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Jukka Jylänki's wonderful bin packing code (http://clb.demon.fi/files/RectangleBinPack.pdf)
#include "Rect.h"
#include "SkylineBinPack.h"

// Kazuho Oku's PicoJSON (https://github.com/kazuho/picojson)
#include "picojson.h"

// Freetype GL functions
#include "distance_map.h"

#include "fbitmap.h"


// Define VERBOSENESS to get too much output.
#undef VERBOSENESS


inline void utf_append(uint32_t cp, std::string & result) {
    char s[8];
    char *e = utf8::append(cp, s);
    for(char *p=s; p<e; ++p) {
        result.push_back(*p);
    }
}

// A little wrapper class to handle FreeType resource cleanup and whatnot.
// I don't love these, but they tend to keep things simpler later.
// Plus, like Bjarne says: RAII is the way to go!  :)

class ftwrapper {
public:
    FT_Library library;
    FT_Face face;
    FT_Error error;
    bool valid;
    
    inline void check_fterr() {
        if( error )
        {
            valid = false;
            fprintf( stderr, "FT_Error (code 0x%02x) : %s\n",
                    FT_Errors[error].code, FT_Errors[error].message);
        }
    }
    
    ftwrapper(std::string fontfile):valid(true) {
        this->error = FT_Init_FreeType( &this->library );
        check_fterr();
        if( error ) { exit(1); }

        this->error = FT_New_Face( this->library, fontfile.c_str(), 0, &this->face );
        check_fterr();
        if( error ) { exit(1); }

        this->error = FT_Select_Charmap( this->face, FT_ENCODING_UNICODE );
        check_fterr();
        if( error ) { exit(1); }
    }
    
    ~ftwrapper() {
        FT_Done_Face( face );
        FT_Done_FreeType( library );
    }
    
    inline bool set_pixel_size(int pixel_size) {
        if(valid) {
            error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
            check_fterr();
        }
        return (valid && !error);
    }
    
    inline bool load_glyph(FT_UInt glyph_index) {
        if(valid) {
            error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT);
            check_fterr();
        }
        return (valid && !error);
    }
    
    inline bool render_glyph() {
        if(valid) {
            error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
            check_fterr();
        }
        return (valid && !error);
    }
    
    inline FT_UInt get_char_index(FT_ULong charcode ) {
        if(valid)
            return FT_Get_Char_Index( face, charcode );
        else
            return 0;
    }

    inline FT_GlyphSlot glyph() {
        return face->glyph;
    }
    
    inline FT_Size_Metrics get_metrics() {
        return face->size->metrics;
    }
    
    inline bool get_kerning( FT_UInt prev_index, FT_UInt cur_index, FT_Vector & kerning ) {
        return !FT_Get_Kerning(face, prev_index, cur_index, FT_KERNING_DEFAULT, &kerning );
    }

private:
    ftwrapper&  operator = (const ftwrapper& ftw);
    ftwrapper(const ftwrapper& ftw);
};





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
    
    inline void scale (float factor) {
        advance_x *= factor;
        bearing_x *=factor;
        bearing_y *=factor;
        bbox_width *=factor;
        bbox_height *=factor;
    };
    
};

/** 
 * loads a glyph from FreeType.
 * @param face a FreeType2 font face
 * @param charcode a character code
 * @param font_size font size in pixels
 * @param sdf_scale scale to use for the Signed Distance Field calculation
 * This function scales the face size to the font_size*sdf_scale, loads
 * the glyph bitmap, and creates a signed distance field based on the 
 * large bitmap. It returns a glyph filled with the scaled-down glyph
 * metrics and the scaled-down (resampled) signed distance field.
 */
glyph load_glyph(ftwrapper & ftw, FT_ULong charcode, int font_size, int sdf_scale) {
        
    // retrieve glyph index from character code
    FT_UInt glyph_index = ftw.get_char_index( charcode );
    
    if(glyph_index == 0) {
        std::string errstr;
        utf_append(charcode, errstr);
        std::cerr << "Consistency error: " << errstr << " index is zero!" << std::endl;
        exit(0);
    }
    
    // load glyph image into the slot, erasing previous image
    
    ftw.set_pixel_size(font_size*sdf_scale);
    ftw.load_glyph(glyph_index);
    ftw.render_glyph();

    glyph new_glyph;
    
    new_glyph.charcode = charcode;
        
    fbitmap<unsigned char> bmp(ftw.glyph()->bitmap.width, ftw.glyph()->bitmap.rows, (unsigned char)0);

    // Copy the face bitmap into the bmp data. Freetype returns bitmaps with "pitch",
    // which is the number of bytes per row and which might be larger than the width.

    int ptch = ftw.glyph()->bitmap.pitch;
    unsigned char *buf = ftw.glyph()->bitmap.buffer;
    
    for( int b_row = 0; b_row < ftw.glyph()->bitmap.rows; ++b_row )
    {
        for( int b_col = 0; b_col < ftw.glyph()->bitmap.width; ++b_col )
        {
            int row_in_array = (ftw.glyph()->bitmap.rows-1)-b_row;
            unsigned char buf_byte = buf[row_in_array*ptch+b_col];
            fbmp::set(bmp, b_col, b_row, buf_byte);
        }
    }
    
    // Create a reasonable padding value...
    
    const int final_x_pad = std::sqrt(font_size);
    const int final_y_pad = std::sqrt(font_size);
    
    const int master_x_pad = final_x_pad*2;
    const int master_y_pad = final_y_pad*2;

    if (sdf_scale==1) {
        
        // Just create and return a dummy (padded) bitmap to use in packing.
        
        fbitmap<unsigned char> lo_bmp(bmp.width + final_x_pad*2, bmp.height + final_y_pad*2, (unsigned char)0);
        
        new_glyph.bmp = lo_bmp;
        new_glyph.bbox_height += final_y_pad*2; // we pad on both sides of the bmp...
        new_glyph.bbox_width += final_x_pad*2;  // I only make this note because I forgot why I multiplied times 2 :)
        
        // We don't care about the rest of the glyph metrics, they aren't used...
        
        return new_glyph;
        
    } else {
        
        // The bitmap loaded above (new_glyph.bmp) is a hires bitmap. Render at high-res,
        // then downsample into a bmp reduced by the scale factor.
        
        fbitmap<double> sdf_bmp;
        
        {
            int x_pad =master_x_pad*sdf_scale;
            int y_pad =master_y_pad*sdf_scale;
            
            sdf_bmp.height = bmp.height+y_pad*2;
            sdf_bmp.width = bmp.width+x_pad*2;
            fbmp::clear(sdf_bmp, 0.0);
            
            // Copy high resolution bitmap with padding and normalize values
            for( int j=0; j < bmp.height; ++j )
            {
                for( int i=0; i < bmp.width; ++i )
                {
                    fbmp::set(sdf_bmp, i+x_pad, j+y_pad, fbmp::get(bmp, i, j)/255.0);
                }
            }
            
        }
        
        // Compute distance map
        make_distance_map( sdf_bmp.data.data() , sdf_bmp.width , sdf_bmp.height );
        
        // Allocate low resolution buffer:
        fbitmap<double> d_bmp;
        d_bmp.height = ftw.glyph()->bitmap.rows/sdf_scale + master_x_pad*2;
        d_bmp.width = ftw.glyph()->bitmap.width/sdf_scale + master_y_pad*2;
        fbmp::clear(d_bmp, 0.0);
        
        // Scale down highres buffer into lowres buffer
        resize( sdf_bmp.data.data(), sdf_bmp.width , sdf_bmp.height,
               d_bmp.data.data(), d_bmp.width, d_bmp.height );
        
        // Convert the (double *) lowres buffer into a (unsigned char *) buffer and
        // rescale values between 0 and 255.
        fbitmap<unsigned char> lo_bmp;
        lo_bmp.height = ftw.glyph()->bitmap.rows/sdf_scale + final_y_pad*2 ;
        lo_bmp.width = ftw.glyph()->bitmap.width/sdf_scale + final_x_pad*2 ;
        fbmp::clear(lo_bmp, (unsigned char)0);
        
        int x_pad_diff = master_x_pad - final_x_pad;
        int y_pad_diff = master_y_pad - final_y_pad;

        for( int j=0; j < (lo_bmp.height); ++j )
        {
            for( int i=0; i < (lo_bmp.width); ++i )
            {
                double v = fbmp::get(d_bmp, i+x_pad_diff, j+y_pad_diff);
                fbmp::set(lo_bmp, i, j, (unsigned char)std::round(255*(1.0-v)));
            }
        }
        
        new_glyph.bmp = lo_bmp;
        
        new_glyph.bearing_x = ftw.glyph()->bitmap_left; // current pen to leftmost border of bitmap, in pixels
        new_glyph.bearing_y = ftw.glyph()->bitmap_top; // current pen to top of bitmap, in pixels

        // Distances are expressed in 26.6 grid-fitted pixels (which means that the values are
        // multiples of 64). For scalable formats, this means that the design kerning distance
        // is scaled, then rounded.
        new_glyph.advance_x = ftw.glyph()->advance.x/64.0f;        // expressed in 1/64th of pixels
        new_glyph.bbox_width = ftw.glyph()->metrics.width/64.0f;   // expressed in 1/64th of pixels
        new_glyph.bbox_height = ftw.glyph()->metrics.height/64.0f; // expressed in 1/64th of pixels
        
        // Scale down dimensions by sdf_scale...
        
        new_glyph.scale(1.0f/(float)sdf_scale);
        
        // Add padding to dimensions...
        
        new_glyph.bearing_x -= final_x_pad;
        new_glyph.bearing_y += final_y_pad;
       
        new_glyph.bbox_width += final_x_pad*2;
        new_glyph.bbox_height += final_y_pad*2;
        
        // Resize dimensions to be percentage of font size, rather than absolute pixels...
        
        new_glyph.scale(1.0f/(float)font_size);
        
        // Set texture coords to 0. The bin packer fills them in.
    
        new_glyph.s0 = 0;
        new_glyph.t0 = 0;
        new_glyph.s1 = 0;
        new_glyph.t1 = 0;
        
    }

    return new_glyph;
};

/**
 * Load all glyphs with character codes in v_charcodes from a font face.
 */
std::map<uint32_t, glyph> load_glyphs(ftwrapper & ftw,
                                      int font_size,
                                      int sdf_scale,
                                      std::vector<uint32_t> const & v_charcodes) {
    
    std::map<uint32_t, glyph> glyphs;
        
    for(int i = 0; i<v_charcodes.size(); ++i) {
        FT_ULong charcode = v_charcodes[i];
        if(sdf_scale>1) {
            std::string ccode;
            utf_append(charcode, ccode);
            std::cout << "Loading 0x" << std::hex << charcode << std::dec << "' (" << ccode << ")..." << std::endl;
        }
        glyph new_glyph = load_glyph(ftw, charcode, font_size, sdf_scale);
        glyphs[new_glyph.charcode] = new_glyph;
    }
    
    return glyphs;
}


/**
 * Packs a bitmap (final_bitmap) using rectangles from a set (well, a map) of glyphs.
 */

bool pack_bin (std::map<uint32_t, glyph> & glyphs,
               fbitmap<unsigned char> & final_bitmap,
               std::vector<uint32_t> const & v_charcodes,
               bool print_stats) {

    bool packed_successfully = false;
    
    fbmp::clear(final_bitmap, (unsigned char)0);
    
    int numToPack = v_charcodes.size();
    
    int numPacked = 0; // The number of rectangles packed successfully, for printing statistics at the end.
    
    // FONT PACKER.
    
    SkylineBinPack bin(final_bitmap.width, final_bitmap.height, false);
    
    // GRAB GLYPHS
    
#ifdef VERBOSENESS
    std::cout << "Packing bitmap size " << final_bitmap.width << "." << std::endl;
#endif
    
    bool in_process = true;
    packed_successfully = false;
    
    for(int i = 0; i<v_charcodes.size(); i+=1) {
        FT_ULong charcode = v_charcodes[i];
        
        glyph& g = glyphs[charcode];
        
        std::string ccode;
        utf_append(g.charcode, ccode);
        
#ifdef VERBOSENESS
        {
            std::cout << "*** Begin Char '" << ccode << "' ***" << std::endl;
            std::cout << "Charcode: '0x" << std::hex << g.charcode << std::dec << "'." << std::endl;
            std::cout << "BBox Width: " << g.bbox_width << ", BBox Height: " << g.bbox_height << std::endl;
            std::cout << "Bmp Width: " << g.bmp.width << ", Bmp Height: " << g.bmp.height << std::endl;
        }
#endif
        
        // Pack the next rectangle in the input list.
        // See more options for this in PackTest.cpp.
        Rect output = bin.Insert(g.bmp.width, g.bmp.height, SkylineBinPack::LevelBottomLeft);
        
        if (output.height == 0 && g.bmp.height>0) // If the packer returns a degenerate height rect, it means the packing failed.
        {
#ifdef VERBOSENESS
            std::cout   << "Failed to pack '" << ccode
            << "' (" << g.bmp.width << ", " << g.bmp.height
            << ") into the bin." << std::endl;
#endif
            in_process = false;
            break;
        }
        else // succeeded.
        {
#ifdef VERBOSENESS
            std::cout << "BMP Width: " << g.bmp.width << ", Height: " << g.bmp.height << std::endl;
#endif
                       
            // x tex coordinate of top-left corner (0.0 to 1.0)
            float s0 = (float)(output.x)/float(final_bitmap.width);
            
            // y tex coordinate of top-left corner (0.0 to 1.0)
            float t0 = (float)(output.y + output.height)/float(final_bitmap.height);
            
            // x tex coordinate of bottom-right corner (0.0 to 1.0)
            float s1 = (float)(output.x + output.width)/float(final_bitmap.width);
            
            // y tex coordinate of bottom-right corner (0.0 to 1.0)
            float t1 = (float)(output.y)/float(final_bitmap.height);
            
#ifdef VERBOSENESS
            std::cout << "Packed rect x: " << output.x << ", y: " << output.y
            << ", width: " << output.width << ", height: " << output.height << "." << std::endl;
#endif
            numPacked+=1;
            
            if(!fbmp::replace_part(final_bitmap, g.bmp, output.x, output.y)) {
                std::cout << "Fatal error: pack into final bitmap failed!" << std::endl;
                exit(1);
            } else {
                g.s0 = s0;
                g.t0 = t0;
                g.s1 = s1;
                g.t1 = t1;
            }
        }
#ifdef VERBOSENESS
        std::cout << "*** End Char '0x" << std::hex << g.charcode << std::dec << "' ***" << std::endl;
#endif
        
        if(false == in_process) {
            // start over with bigger bin
            //std::cerr << "Breaking." << std::endl;
            break;
        }
        
    }
    
    if(print_stats) {
        std::cout << "Packed " << numPacked << " rectangles out of "
        << numToPack << " into a bin of size " << final_bitmap.width
        << "x" << final_bitmap.height << "." << std::endl;
        
        std::cout << "Bin occupancy: " << (bin.Occupancy() * 100.f) << "%." << std::endl;
    }
    
    if(numPacked==numToPack) {
        packed_successfully = true;
    }
    
    return packed_successfully;
}

std::string file_to_font_name(std::string filename) {
#ifdef _WIN32
    char delimiter = '\\';
#else
    char delimiter = '/';
#endif
    
    int separator_loc = filename.find_last_of(delimiter);
    
    int name_start = 0;
    if(std::string::npos != separator_loc) {
        name_start = separator_loc+1;
    }
    
    int name_end = filename.find_last_of('.') - name_start;
    
    std::string font_name = filename.substr(name_start, name_end);
    
    return font_name;
}


int main( int argc, char **argv )
{
    
    std::vector<uint32_t> v_charcodes;
    
    std::string font_filename;
    int bitmap_size;

    // *** Process Args
    
    if(argc!=3) {
        std::cerr << "Arguments required: '" << argv[0] << " fontname.ttf bitmap_size'" << std::endl;
        exit(0);
    } else {
        font_filename = argv[1];
        bitmap_size = std::atoi(argv[2]);
    }

    // *** Load Font
    
    ftwrapper ftw(font_filename);
    
    if (! ftw.valid ) {
        exit(0);
    }
    
    // *** Find valid character codes
    
    {
        
        std::vector<uint32_t> global_charcodes;
        
        for(uint32_t i=' ';i<='~';i+=1) {
            global_charcodes.push_back(i);
        }
        
        global_charcodes.push_back(0x2026); // ellipsis
        global_charcodes.push_back(0x20AC); // Euro
        global_charcodes.push_back(0x00A9); // Copyright symbol
        global_charcodes.push_back(0x201C); // double opening quote
        global_charcodes.push_back(0x201D); // double closing quote
        global_charcodes.push_back(0x2018); // single opening quote
        global_charcodes.push_back(0x2019); // single closing quote
        
        for(int i = 0; i<global_charcodes.size(); i+=1) {
            FT_ULong gcharcode = global_charcodes[i];
            
            // retrieve glyph index from character code
            FT_UInt glyph_index = ftw.get_char_index( gcharcode );
            if(glyph_index == 0) {
                std::string errstr;
                utf_append(gcharcode, errstr);
                std::cerr << "Character '" << errstr << "' index is zero. Will not render." << std::endl;
            } else {
                v_charcodes.push_back(gcharcode);
            }
        }
        
    }
    
    // *** Pack Glyphs
    
    fbitmap<unsigned char> final_bitmap(bitmap_size, bitmap_size, (unsigned char)0);

    int font_size = 2;
    
    bool packed_successfully = false;
    
    std::map<uint32_t, glyph> m_glyphs;

    do {
        font_size += 2;
        m_glyphs = load_glyphs(ftw, font_size, 1, v_charcodes);
        packed_successfully = pack_bin (m_glyphs, final_bitmap, v_charcodes, false);

    } while(packed_successfully);

    if(font_size == 4) {
        std::cerr << "Font packing failure. Pack failed at " << font_size << " pixels. Stopping." << std::endl;
        exit(1);
    } else {
        font_size-=2;
        
        // Okay, we have our good sizes. Now it's time to do the distance mapping...
        
        int scale = 16;
        
        m_glyphs = load_glyphs(ftw, font_size, scale, v_charcodes);
        
        std::cout << "Packing at " << font_size << " pixels." << std::endl;
        packed_successfully = pack_bin (m_glyphs, final_bitmap, v_charcodes, true);
    }
    
    if(!packed_successfully) {
        std::cerr << "Final font packing failure. Pack failed at " << font_size << " pixels. Stopping." << std::endl;
        exit(1);
    }
    
    std::cout << "Succesfully packed at " << font_size << " pixels." << std::endl;
    
    // DONE LOADING
 
    // Reset pixel size to reset metrics (below)...
    ftw.set_pixel_size(font_size);

    // Generate metrics
    
    FT_Size_Metrics metrics = ftw.get_metrics();
    // Values are expressed in 1/64th of pixels.
    float ascender = float(metrics.ascender)/64.0f;
    float descender = float(metrics.descender)/64.0f;
    float height = float(metrics.height)/64.0f;
    float max_advance = float(metrics.max_advance)/64.0f;
    
    float space_advance = 0;
    
    {
        // retrieve space index from character code
        FT_UInt space_index = ftw.get_char_index( ' ' );
        
        if(0==space_index) {
            std::cerr << "Warning: space glyph not found. Approximating space advance." << std::endl;
            space_index = ftw.get_char_index( 'i' ); // lowercase 'i' approximates a space...
        }
        
        if(space_index!=0) {
            ftw.load_glyph(space_index);
            if(!ftw.error) {
                space_advance = ftw.glyph()->advance.x/64.0f;
            }
        }
    }
    
    // Resize metrics to be percentage of font size, rather than absolute pixels
    ascender /= (float)font_size;
    descender /= (float)font_size;
    height /= (float)font_size;
    max_advance /= (float)font_size;
    space_advance /= (float)font_size;
    
    std::map<std::string, float> m_metrics;
        
    m_metrics["ascender"]=ascender;
    m_metrics["descender"]=descender;
    m_metrics["height"]=height;
    m_metrics["max_advance"]=max_advance;
    m_metrics["space_advance"]=space_advance;
    
    // Generate kernings
    
    for(int i = 0; i<v_charcodes.size(); i+=1) {
        FT_ULong gcharcode = v_charcodes[i];
        
        glyph& cur_glyph = m_glyphs[gcharcode];
        FT_UInt glyph_index = ftw.get_char_index( cur_glyph.charcode );
        cur_glyph.kernings.clear();
        
        for(int j=0; j<v_charcodes.size(); j+=1)
        {
            FT_ULong prevcharcode = v_charcodes[j];
            glyph& prev_glyph = m_glyphs[prevcharcode];
            FT_UInt prev_index = ftw.get_char_index( prev_glyph.charcode );
            
            FT_Vector kerning;
            ftw.get_kerning(prev_index, glyph_index, kerning);

            // Default value is FT_KERNING_DEFAULT which has value 0. It corresponds to kerning
            // distances expressed in 26.6 grid-fitted pixels (which means that the values are
            // multiples of 64). For scalable formats, this means that the design kerning distance
            // is scaled, then rounded.
            
            float kern_value = float(kerning.x)/64.0f;
            
            if( abs(kern_value)>.0001f )
            {
                kern_value /= (float)font_size;
                cur_glyph.kernings[prev_glyph.charcode] = kern_value;
            }
        }
    }
     
    // WRITE THE FINAL BITMAP
    
    // the name
    std::string bitmap_name(file_to_font_name(font_filename));
    bitmap_name+=".png";
       
    if(stbi_write_png(bitmap_name.c_str(),
                      final_bitmap.width,
                      final_bitmap.height,
                      1,
                      final_bitmap.data.data(),
                      final_bitmap.width)) {
        std::cout << "Wrote " << bitmap_name << "." << std::endl;
    } else {
        std::cerr << "Write of " << bitmap_name << " FAILED." << std::endl;
    }
    
    // OUTPUT JSON DESCRIPTIONS
    
    {
        float ascender = m_metrics["ascender"];
        float descender = m_metrics["descender"];
        float height = m_metrics["height"];
        float max_advance = m_metrics["max_advance"];
        float space_advance = m_metrics["space_advance"];
        
        picojson::object pjo;

        pjo["name"] = picojson::value(file_to_font_name(font_filename));
        pjo["size"] = picojson::value((float)font_size);
        pjo["height"] = picojson::value(height);
        pjo["ascender"] = picojson::value(ascender);
        pjo["descender"] = picojson::value(descender);
        pjo["max_advance"] = picojson::value(max_advance);
        pjo["space_advance"] = picojson::value(space_advance);
        pjo["bitmap_width"] = picojson::value(float(final_bitmap.width));
        pjo["bitmap_height"] = picojson::value(float(final_bitmap.height));
        
        picojson::object json_glyph_data;
        json_glyph_data.clear();
        
        for(int i = 0; i<v_charcodes.size(); i+=1) {
            FT_ULong gcharcode = v_charcodes[i];
            
            glyph& g = m_glyphs[gcharcode];
            
            picojson::object json_glyph;
            
            std::string ccode;
            utf_append(g.charcode, ccode);
            
            json_glyph["charcode"] = picojson::value(ccode);
            json_glyph["bbox_width"] = picojson::value(g.bbox_width);
            json_glyph["bbox_height"] = picojson::value(g.bbox_height);
            json_glyph["bearing_x"] = picojson::value(float(g.bearing_x));
            json_glyph["bearing_y"] = picojson::value(float(g.bearing_y));
            json_glyph["advance_x"] = picojson::value(g.advance_x);
            json_glyph["s0"] = picojson::value(g.s0);
            json_glyph["t0"] = picojson::value(g.t0);
            json_glyph["s1"] = picojson::value(g.s1);
            json_glyph["t1"] = picojson::value(g.t1);
            
            picojson::object json_kernings;
            
            for (int kern_idx = 0; kern_idx < v_charcodes.size() ; kern_idx+=1){
                glyph& prev_glyph = m_glyphs[v_charcodes[kern_idx]];
                if(g.kernings.count(prev_glyph.charcode)>0) {
                    float kern_val = g.kernings[prev_glyph.charcode];
                    if(kern_val<-.001 || .001<kern_val) {
                        ccode.clear();
                        utf_append(prev_glyph.charcode, ccode);
                        json_kernings[ccode] = picojson::value(kern_val);
                    }
                }
            }
            
            json_glyph["kernings"] = picojson::value(json_kernings);
            ccode.clear();
            utf_append(g.charcode, ccode);
            json_glyph_data[ccode] = picojson::value(json_glyph);
            
        }
        
        pjo["glyph_data"] = picojson::value(json_glyph_data);
        
        std::string str = picojson::value(pjo).serialize();

        {
            std::string v_font_name = file_to_font_name(font_filename);
            std::cout << "Writing " << str.length() << " bytes of JSON data to "
            << v_font_name+std::string(".json") << std::endl;
            
            std::ofstream jsonfile;
            jsonfile.open ((v_font_name+std::string(".json")).c_str());
            jsonfile << str;
            jsonfile.close();
        }
    }
    std::cout << "Successful. Exiting." << std::endl;
    return 0;
}
