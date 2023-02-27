#include "ttfMeshFont.h"

#include <iostream>

#include "gfxAPI/glad/include/glad/glad.h"

#include "gfxAPI/shader.h"

#define GLSL(...) "#version 330\n" #__VA_ARGS__

namespace {
    static GfxAPI::Shader *const getOrCreateShader2d() {

        static GfxAPI::Shader* pShader = nullptr;
        if (pShader != nullptr) { return pShader; }

        // VERTEX SHADER
        const char* vertexShader2dSrc = GLSL(
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
                //gl_Position = u_mvpMatrix * vec4( a_pos * vec2(0.1, 0.1) + vec2( (-0.8+u_x)*0.1-0.8, 0.0 ), -1.0, 1.0 );
                //gl_Position = u_mvpMatrix * vec4( a_pos * vec2(0.1, 0.1) + vec2( (-0.0+u_x)*0.1-0.0, 0.0 ), 0.5, 1.0 );
                gl_Position = u_mvpMatrix * vec4( a_pos, 0.5, 1.0 );
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
            std::make_pair( std::string{vertexShader2dSrc},   GfxAPI::Shader::eShaderStage::VS ),
            std::make_pair( std::string{fragmentShaderSrc}, GfxAPI::Shader::eShaderStage::FS ),
        };

        pShader = new GfxAPI::Shader;

        gfxUtils::createShader( 
            *pShader, 
            shaderDesc
        );
    
        return pShader;
    }

    static GfxAPI::Shader *const getOrCreateShader3d() {

        static GfxAPI::Shader* pShader = nullptr;
        if (pShader != nullptr) { return pShader; }

        // VERTEX SHADER
        const char* vertexShader3dSrc = GLSL(
            layout ( location = 0 ) in vec3 a_pos;
            //layout ( location = 1 ) in vec3 a_norm;

            uniform mat4 u_mvpMatrix;
            uniform float u_x;

            uniform vec4 u_topL;
            uniform vec4 u_btmR;

            out gl_PerVertex{
                vec4 gl_Position;
            };

            void main() {
                //vec4 pos = vec4( a_pos * vec3( 0.1, 0.1, 1.0 ) + vec3( (-0.8 + u_x) * 0.1 - 0.8, 0.4, 0.0 ), 1.0 );
                gl_Position = u_mvpMatrix * vec4( a_pos, 1.0 );
                //gl_Position.z = 0.5;
                //gl_Position = u_mvpMatrix * pos;
            }
        );

        // FRAGMENT SHADER
        const char* fragmentShaderSrc = GLSL(
            out vec4 o_Color;
            void main() {
                o_Color = vec4( 0.9, 0.1, 0.5, 1.0 );
            }
        );

        std::vector< std::pair< gfxUtils::srcStr_t, GfxAPI::Shader::eShaderStage > > shaderDesc{
            std::make_pair( std::string{vertexShader3dSrc},   GfxAPI::Shader::eShaderStage::VS ),
            std::make_pair( std::string{fragmentShaderSrc}, GfxAPI::Shader::eShaderStage::FS ),
        };

        pShader = new GfxAPI::Shader;

        gfxUtils::createShader( 
            *pShader, 
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
            const size_t numBytes = numIndices * sizeof( uint32_t );
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
    getOrCreateShader2d();
    getOrCreateShader3d();

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

const ttfMeshFont::glyphData_t* ttfMeshFont::chooseGlyph(TCHAR symbol) {
    // find a glyph in the font file

    const auto glyphQueryResult = mGlyphs.find( symbol );
    if (glyphQueryResult != mGlyphs.end()) {
        return &glyphQueryResult->second;
    }

    int index = ttf_find_glyph( mpFont, symbol);
    if (index < 0) { return nullptr; }

    // make 2d mesh
    ttf_mesh* pGlyphMesh2d;
    if (ttf_glyph2mesh/*3d*/( &mpFont->glyphs[index], &pGlyphMesh2d, TTF_QUALITY_NORMAL, TTF_FEATURES_DFLT/*, 0.1f*/ ) != TTF_DONE) {
        return nullptr;
    }

    // if successful, release the previous object and save the state

    //ttf_free_mesh3d(mesh);
    //glyph = &mpFont->glyphs[index];
    //mesh = pGlyphMesh2d;

    gfxUtils::bufferHandles_t gfxGlyphBuffer2d;
    memset( &gfxGlyphBuffer2d.vaoHandle, 0, sizeof( gfxGlyphBuffer2d.vaoHandle ) );
    gfxGlyphBuffer2d = createMeshGfxBuffers(
        static_cast<uint32_t>( pGlyphMesh2d->nvert ), //const size_t numVertexCoordVec3s,
        &pGlyphMesh2d->vert->x, //const std::vector< float >& vertexCoordFloats,
        static_cast<uint32_t>( pGlyphMesh2d->nfaces * 3 ), //const size_t numIndices,
        //reinterpret_cast<uint32_t*>( &pGlyphMesh2d->faces->v1 )//const std::vector< uint32_t >& indices 
        &pGlyphMesh2d->faces->v1
    );


    // make 3d object from the glyph
    ttf_mesh3d* pGlyphMesh3d;
    if (ttf_glyph2mesh3d( &mpFont->glyphs[index], &pGlyphMesh3d, TTF_QUALITY_NORMAL, TTF_FEATURES_DFLT, 0.1f ) != TTF_DONE) {
        return nullptr;
    }


    gfxUtils::bufferHandles_t gfxGlyphBuffer3d;
    memset( &gfxGlyphBuffer3d.vaoHandle, 0, sizeof( gfxGlyphBuffer3d.vaoHandle ) );
    gfxGlyphBuffer3d = gfxUtils::createMeshGfxBuffers(
        static_cast<uint32_t>( pGlyphMesh3d->nvert ), //const size_t numVertexCoordVec3s,
        &pGlyphMesh3d->vert->x, //const std::vector< float >& vertexCoordFloats,
        static_cast<uint32_t>( pGlyphMesh3d->nvert ), //const size_t numVertexCoordVec3s,
        &pGlyphMesh3d->normals->x, //const std::vector< float >& vertexCoordFloats,
        static_cast<uint32_t>( pGlyphMesh3d->nfaces * 3 ), //const size_t numIndices,
        reinterpret_cast<uint32_t*>( &pGlyphMesh3d->faces->v1 )//const std::vector< uint32_t >& indices 
        //&pGlyphMesh3d->faces->v1
    );

    //if (glGetError() != GL_NO_ERROR) {
    //    printf( "GL error!\n" );
    //}


    mGlyphs.insert( std::make_pair( symbol, glyphData_t{ &mpFont->glyphs[index], pGlyphMesh2d, pGlyphMesh3d, gfxGlyphBuffer2d, gfxGlyphBuffer3d } ) );
    return &mGlyphs.find( symbol )->second;
}

void ttfMeshFont::renderText2d( const float x, const float y, const TCHAR* pText ) {
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

    auto* pShader = getOrCreateShader2d();
    pShader->use( true );

    linAlg::mat4_t mvpMatrix;
    linAlg::mat4_t projMatrix;
    linAlg::loadOrthoMatrix( projMatrix, -mRatiosXY[0], mRatiosXY[0], -mRatiosXY[1], mRatiosXY[1], -1.0f, 1.0f );
    linAlg::mat4_t fontScaleMatrix;
    linAlg::loadScaleMatrix( fontScaleMatrix, {32.0f, 32.0f, 1.0f, 1.0f} );
    linAlg::multMatrix( mvpMatrix, projMatrix, fontScaleMatrix );

    //linAlg::loadIdentityMatrix( mvpMatrix );
    pShader->setMat4( "u_mvpMatrix", mvpMatrix );

    float currX = x;

#if 0

    std::vector<const glyphData_t*> glyphs;
    for (; *pText; ++pText) {
        if (*pText >= 32 && *pText < 128) {
            const auto *charGlyph = chooseGlyph( *pText );
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
            const auto& charGlyph = chooseGlyph( *pText );
            
            pShader->setFloat( "u_x", currX );
            
            if (charGlyph == nullptr) { currX += 1.0; continue; }
            currX += charGlyph->pGlyph->advance;
            
            glPolygonMode(GL_FRONT_AND_BACK,  GL_FILL);
            glBindVertexArray( charGlyph->bufferHandle2d.vaoHandle );
            glDrawElements( GL_TRIANGLES, charGlyph->pMesh2d->nfaces * 3, GL_UNSIGNED_INT, nullptr );
            //textShader.setVec4( "u_topL", { q.x0, q.y0, q.s0, q.t0 } );
            //textShader.setVec4( "u_btmR", { q.x1, q.y1, q.s1, q.t1 } );
        }
    }
#endif

    glBindVertexArray( 0 );
    pShader->use( false );

}


void ttfMeshFont::renderText3d( const float x, const float y, const TCHAR* pText ) {
    glDisable( GL_CULL_FACE );

    auto* pShader = getOrCreateShader3d();
    pShader->use( true );

    linAlg::mat4_t mvpMatrix;
    linAlg::mat4_t projMatrix;
    linAlg::loadOrthoMatrix( projMatrix, -mRatiosXY[0], mRatiosXY[0], -mRatiosXY[1], mRatiosXY[1], -1.0f, 1.0f );
    linAlg::mat4_t fontScaleMatrix;
    linAlg::loadScaleMatrix( fontScaleMatrix, {32.0f, 32.0f, 1.0f, 1.0f} );
    linAlg::multMatrix( mvpMatrix, projMatrix, fontScaleMatrix );

    //linAlg::loadIdentityMatrix( mvpMatrix );
    pShader->setMat4( "u_mvpMatrix", mvpMatrix );

    float currX = x;

    //glPolygonMode(GL_FRONT_AND_BACK,  GL_FILL);

    for (; *pText; ++pText) {
        if (*pText >= 32 && *pText < 128) {
            const auto& charGlyph = chooseGlyph( *pText );

            pShader->setFloat( "u_x", currX );

            if (charGlyph == nullptr) { currX += 1.0; continue; }
            currX += charGlyph->pGlyph->advance;

            glBindVertexArray( charGlyph->bufferHandle3d.vaoHandle );
            glDrawElements( GL_TRIANGLES, charGlyph->pMesh3d->nfaces * 3, GL_UNSIGNED_INT, nullptr );
        }
    }

    glBindVertexArray( 0 );
    pShader->use( false );
}
