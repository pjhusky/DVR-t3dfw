#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stbFont.h"

#include "gfxAPI/glad/include/glad/glad.h"

#include "gfxAPI/apiAbstractions.h"
#include "gfxAPI/texture.h"

namespace {
    static gfxUtils::bufferHandles_t    textQuadBuffer;
    static GfxAPI::Shader               textShader;
}

void stbFont::initStbFontRendering()
{
    fread(ttf_buffer, 1, 1<<24, fopen("./data/fonts/Skinny__.ttf", "rb"));
    //fread(ttf_buffer, 1, 1<<24, fopen("./data/fonts/Stylish-Regular.ttf", "rb"));
    //fread(ttf_buffer, 1, 1<<24, fopen("./data/fonts/Spectral-Regular.ttf", "rb"));
    stbtt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap,512,512, 32,96, cdata); // no guarantee this fits!
    // can free ttf_buffer at this point
    //// can free temp_bitmap at this point

    GfxAPI::Texture::Desc_t fontTexDesc{
        .texDim = { 512, 512, 0 },
        .numChannels = 1,
        .channelType = GfxAPI::eChannelType::i8,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    fontTex2d = new GfxAPI::Texture;
    fontTex2d->create( fontTexDesc );
    fontTex2d->uploadData( temp_bitmap, GfxAPI::toApiFormatFromNumChannels(1), GfxAPI::toApiDataChannelType( GfxAPI::eChannelType::u8 ), 0 );

    textQuadBuffer = gfxUtils::createScreenQuadGfxBuffers();

    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > textShaderDesc{
        std::make_pair( "./src/shaders/texturedQuad.vert.glsl.preprocessed", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/texturedQuad.frag.glsl.preprocessed", GfxAPI::Shader::eShaderStage::FS ),
    };
    gfxUtils::createShader( textShader, textShaderDesc );
    textShader.use( true );
    textShader.setInt( "u_Tex", 5 );
    textShader.setVec4( "u_Color", { 0.99f, 0.99f, 0.99f, 0.75f} );
    textShader.use( false );
}

void stbFont::renderStbFontText(float x, float y, const char* text) {
    glDisable( GL_CULL_FACE );

    // NOPE assume orthographic projection with units = screen pixels, origin at top left
    // assume coords in NDC

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    fontTex2d->bindToTexUnit( 5 );

    textShader.use( true );

    linAlg::mat4_t orthoMat;
    //linAlg::loadPerspectiveMatrix( orthoMat, 0.0f, fbWidth,  )
    //glViewport( 0, 0, 2400, 1800 );
    //linAlg::loadPerspectiveMatrix( orthoMat, 512.0f, -512.0, -512.0f, 512.0f, -1.0f, 1.0f );
    linAlg::loadOrthoMatrix( orthoMat, -512.0f, 512.0, 512.0f, -512.0f, -1.0f, 1.0f );
    linAlg::mat4_t orthoMatT;
    linAlg::transpose( orthoMatT, orthoMat );
    textShader.setMat4( "u_trafoMatrix", orthoMat );

    glBindVertexArray( textQuadBuffer.vaoHandle );
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad( cdata, 512, 512, *text - 32, &x, &y, &q, 1 );//1=opengl & d3d10+,0=d3d9

            //textShader.setVec4( "u_topL", { q.x0/512.0f, -q.y0/512.0f, q.s0, q.t0 } );
            //textShader.setVec4( "u_btmR", { q.x1/512.0f, -q.y1/512.0f, q.s1, q.t1 } );
            textShader.setVec4( "u_topL", { q.x0, q.y0, q.s0, q.t0 } );
            textShader.setVec4( "u_btmR", { q.x1, q.y1, q.s1, q.t1 } );

            glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0 );
        }
        ++text;
    }
    glBindVertexArray( 0 );
    textShader.use( false );

    fontTex2d->unbindFromTexUnit();

    glEnable( GL_CULL_FACE );
}