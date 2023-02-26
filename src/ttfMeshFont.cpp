#include "ttfMeshFont.h"

#include <iostream>

#include "gfxAPI/glad/include/glad/glad.h"

#include "gfxAPI/shader.h"

#define GLSL(...) "#version 330\n" #__VA_ARGS__

namespace {
    static GfxAPI::Shader *const getOrCreateShader() {

        static GfxAPI::Shader* pShader = nullptr;
        if (pShader != nullptr) { return pShader; }

        // VERTEX SHADER
        const char* vertexShaderSrc = GLSL(
            layout ( location = 0 ) in vec2 a_pos;
            //layout ( location = 1 ) in vec2 a_norm;

            uniform mat4 u_mvpMatrix;
            uniform float u_x;

            uniform vec4 u_topL;
            uniform vec4 u_btmR;

            out gl_PerVertex{
                vec4 gl_Position;
            };

            void main() {
                gl_Position = u_mvpMatrix * vec4( a_pos * vec2(0.1, 0.1) + vec2( (-0.8+u_x)*0.1-0.8, 0.0 ), -1.0, 1.0 );
            }
        );

        // FRAGMENT SHADER
        const char* fragmentShaderSrc = GLSL(
            out vec4 o_Color;
            void main() {
                o_Color = vec4( 0.5, 1.0, 0.1, 1.0 );
            }
        );

        std::vector< std::pair< gfxUtils::srcStr_t, GfxAPI::Shader::eShaderStage > > shaderDesc{
            std::make_pair( std::string{vertexShaderSrc},   GfxAPI::Shader::eShaderStage::VS ),
            std::make_pair( std::string{fragmentShaderSrc}, GfxAPI::Shader::eShaderStage::FS ),
        };

        pShader = new GfxAPI::Shader;

        gfxUtils::createShader( 
            *pShader, 
            //std::vector< std::pair< std::string, GfxAPI::Shader::eShaderStage > >{ 
            //    std::make_pair( std::string{vertexShaderSrc},   GfxAPI::Shader::eShaderStage::VS ), 
            //    std::make_pair( std::string{fragmentShaderSrc}, GfxAPI::Shader::eShaderStage::FS ) } 
            shaderDesc
        );
    
        return pShader;
    }

    gfxUtils::bufferHandles_t createMeshGfxBuffers(
        const uint32_t numVertexCoordVec2s,
        const float* const pVertexCoordFloats,
        const uint32_t numIndices,
        const int32_t* const pIndices ) {


        //if (glGetError() != GL_NO_ERROR) {
        //    printf( "GL error!\n" );
        //}

        uint32_t stlModel_VAO;
        glGenVertexArrays(1, &stlModel_VAO);

        uint32_t stlCoords_VBO;
        glGenBuffers(1, &stlCoords_VBO);

        uint32_t stlModel_EBO;
        glGenBuffers(1, &stlModel_EBO);

        glBindVertexArray(stlModel_VAO);

        glBindBuffer(GL_ARRAY_BUFFER, stlCoords_VBO);
        const size_t numBytes = numVertexCoordVec2s * 2 *sizeof( float );
        glBufferData(GL_ARRAY_BUFFER, numBytes, pVertexCoordFloats, GL_STATIC_DRAW);
        const uint32_t attribIdx = 0;
        const int32_t components = 2;
        glVertexAttribPointer(attribIdx, components, GL_FLOAT, GL_FALSE, 0, 0); // the last two zeros mean "tightly packed"
        glEnableVertexAttribArray(attribIdx);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, stlModel_EBO);
        {
            const size_t numBytes = numIndices * 3 * sizeof( uint32_t );
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBytes, pIndices, GL_STATIC_DRAW);
        }

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        gfxUtils::bufferHandles_t handles;
        memset( &handles.vaoHandle, 0, sizeof( handles.vaoHandle ) );
        memset( &handles.eboHandle, 0, sizeof( handles.eboHandle ) );
        
        handles.vaoHandle = stlModel_VAO;
        handles.vboHandles = std::vector< uint32_t >{ stlCoords_VBO/*, (uint32_t)-1*/ };
        handles.eboHandle = stlModel_EBO;

        //if (glGetError() != GL_NO_ERROR) {
        //    printf( "GL error!\n" );
        //}

        return handles;        
    }

}

ttfMeshFont::ttfMeshFont( const std::filesystem::path& fileUrl ) {
    
    //if (glGetError() != GL_NO_ERROR) {
    //    printf( "GL error!\n" );
    //}

    //mpFont = new ttf_t;
    ttf_load_from_file(fileUrl.string().c_str(), &mpFont, false);
    //ttf_free_list(list);
    //if (font == nullptr) return false;
    getOrCreateShader();

    //if (glGetError() != GL_NO_ERROR) {
    //    printf( "GL error!\n" );
    //}

}

ttfMeshFont::~ttfMeshFont() {
    for (auto& entry : mGlyphs) {
        //ttf_free_mesh3d( entry.second.pMesh );
        ttf_free_mesh( entry.second.pMesh2d );
    }
    //delete mpFont;
}

const ttfMeshFont::glyphData_t* ttfMeshFont::choose_glyph(TCHAR symbol) {
    // find a glyph in the font file

    const auto glyphQueryResult = mGlyphs.find( symbol );
    if (glyphQueryResult != mGlyphs.end()) {
        return &glyphQueryResult->second;
    }

    int index = ttf_find_glyph( mpFont, symbol);
    if (index < 0) { return nullptr; }

    // make 3d object from the glyph

    //ttf_mesh3d_t *pGlyphMesh;
    ttf_mesh* pGlyphMesh;
    if (ttf_glyph2mesh/*3d*/( &mpFont->glyphs[index], &pGlyphMesh, TTF_QUALITY_NORMAL, TTF_FEATURES_DFLT/*, 0.1f*/ ) != TTF_DONE) {
        return nullptr;
    }

    // if successful, release the previous object and save the state

    //ttf_free_mesh3d(mesh);
    //glyph = &mpFont->glyphs[index];
    //mesh = pGlyphMesh;

    

    gfxUtils::bufferHandles_t gfxGlyphBuffer;
    memset( &gfxGlyphBuffer.vaoHandle, 0, sizeof( gfxGlyphBuffer.vaoHandle ) );

    gfxGlyphBuffer = createMeshGfxBuffers(
        static_cast<uint32_t>( pGlyphMesh->nvert ), //const size_t numVertexCoordVec3s,
        &pGlyphMesh->vert->x, //const std::vector< float >& vertexCoordFloats,
        static_cast<uint32_t>( pGlyphMesh->nfaces ), //const size_t numIndices,
        //reinterpret_cast<uint32_t*>( &pGlyphMesh->faces->v1 )//const std::vector< uint32_t >& indices 
        &pGlyphMesh->faces->v1
    );

    //if (glGetError() != GL_NO_ERROR) {
    //    printf( "GL error!\n" );
    //}


    mGlyphs.insert( std::make_pair( symbol, glyphData_t{ &mpFont->glyphs[index], pGlyphMesh, gfxGlyphBuffer } ) );
    return &mGlyphs.find( symbol )->second;
}

void ttfMeshFont::renderText( const float x, const float y, const TCHAR* pText ) {
    //glTranslatef(width / 2, height / 10, 0);
    //glScalef(0.9f * height, 0.9f * height, 1.0f);
    //glScalef(1.0f, 1.0f, 0.1f);
    //glTranslatef(-(glyph->xbounds[0] + glyph->xbounds[1]) / 2, -glyph->ybounds[0], 0.0f);

    //glVertexPointer(2, GL_FLOAT, 0, &mesh->vert->x);
    //glDrawElements(GL_TRIANGLES, mesh->nfaces * 3,  GL_UNSIGNED_INT, &mesh->faces->v1);

    //if (glGetError() != GL_NO_ERROR) {
    //    printf( "GL error!\n" );
    //}


    glDisable( GL_CULL_FACE );

    auto* const pShader = getOrCreateShader();
    pShader->use( true );
    linAlg::mat4_t mvpMatrix;
    //linAlg::loadOrthoMatrix( mvpMatrix, -512.0f, 512.0f, -512.0f, 512.0f, 0.1f, 10000.0f );
    linAlg::loadIdentityMatrix( mvpMatrix );
    pShader->setMat4( "u_mvpMatrix", mvpMatrix );


    //if (glGetError() != GL_NO_ERROR) {
    //    printf( "GL error!\n" );
    //}
    //GLenum errorCode;
    //while ( ( errorCode = glGetError() ) != GL_NO_ERROR ) {
    //    std::string error;
    //    switch ( errorCode ) {
    //    case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
    //    case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
    //    case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
    //    case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
    //    case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
    //    case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
    //    case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
    //    }
    //    std::cout << error << " | "  << " (" <<  ")" << std::endl; 
    //    std::cout << std::flush;
    //}

    float currX = x;

#if 1

    std::vector<const glyphData_t*> glyphs;
    for (; *pText; ++pText) {
        if (*pText >= 32 && *pText < 128) {
            const auto *charGlyph = choose_glyph( *pText );
            //if (charGlyph == nullptr) { continue; }
            glyphs.push_back( charGlyph );
        }
    }

    for (const auto* pCharGlyph : glyphs) {
        pShader->setFloat( "u_x", currX );
        if (pCharGlyph == nullptr) { currX += 1.0; continue; }
        currX += pCharGlyph->pGlyph->advance;

        //printf( "pCharGlyph->bufferHandle2d.vaoHandle = %p\t", pCharGlyph->bufferHandle2d.vaoHandle );
        //printf( "pCharGlyph->pMesh2d->nfaces = %d\n", pCharGlyph->pMesh2d->nfaces );
        
        glBindVertexArray( pCharGlyph->bufferHandle2d.vaoHandle );
        glDrawElements( GL_TRIANGLES, pCharGlyph->pMesh2d->nfaces * 3, GL_UNSIGNED_INT, nullptr );        
        
        
    }

#elif 1 // doesn't render font in Release

    for (; *pText; ++pText) {
        if (*pText >= 32 && *pText < 128) {
            const auto& charGlyph = choose_glyph( *pText );
            
            pShader->setFloat( "u_x", currX );
            
            if (charGlyph == nullptr) { currX += 1.0; continue; }
            currX += charGlyph->pGlyph->advance;
            
            glPolygonMode(GL_FRONT_AND_BACK,  GL_FILL);
            glBindVertexArray( charGlyph->bufferHandle2d.vaoHandle );
            glDrawElements( GL_TRIANGLES, charGlyph->pMesh2d->nfaces * 3, GL_UNSIGNED_INT, nullptr );
            //textShader.setVec4( "u_topL", { q.x0, q.y0, q.s0, q.t0 } );
            //textShader.setVec4( "u_btmR", { q.x1, q.y1, q.s1, q.t1 } );

            //glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0 );

            
            

        }
        //break;
    }
#endif

    glBindVertexArray( 0 );
    pShader->use( false );

}
