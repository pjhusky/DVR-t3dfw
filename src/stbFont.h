#ifndef _STB_FONT_H_CB0C8EB4_1C22_4A31_A271_7EF72499E949
#define _STB_FONT_H_CB0C8EB4_1C22_4A31_A271_7EF72499E949

#include "external/stb/stb_truetype.h"

#include "gfxAPI/shader.h"
#include "gfxUtils.h"

#include "math/linAlg.h"

#include <filesystem>


namespace GfxAPI {
    struct Texture;
}


//namespace linAlg {
//    struct vec4_t;
//}

struct stbFont {

    stbFont( const std::filesystem::path& fontPath ) { initFontRendering( fontPath ); }
    void renderText( float x, float y, const linAlg::vec4_t& color, const char* text );

    void setAspectRatios( const linAlg::vec2_t& xyRatios ) { mRatiosXY = xyRatios; }

private:

    void initFontRendering( const std::filesystem::path& fontPath );

    unsigned char ttf_buffer[1<<24];
    unsigned char temp_bitmap[512*512];

    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
    GfxAPI::Texture*                    fontTex2d;

    linAlg::vec2_t  mRatiosXY;
};

#endif // _STB_FONT_H_CB0C8EB4_1C22_4A31_A271_7EF72499E949
