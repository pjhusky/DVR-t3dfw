#ifndef _STB_FONT_H_CB0C8EB4_1C22_4A31_A271_7EF72499E949
#define _STB_FONT_H_CB0C8EB4_1C22_4A31_A271_7EF72499E949

#include "external/stb/stb_truetype.h"

#include "gfxAPI/shader.h"
#include "gfxUtils.h"

namespace GfxAPI {
    struct Texture;
}

struct stbFont {

    stbFont() { initStbFontRendering(); }
    void renderStbFontText( float x, float y, const char* text );

private:

    void initStbFontRendering();

    unsigned char ttf_buffer[1<<24];
    unsigned char temp_bitmap[512*512];

    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
    GfxAPI::Texture*                    fontTex2d;

};

#endif // _STB_FONT_H_CB0C8EB4_1C22_4A31_A271_7EF72499E949
