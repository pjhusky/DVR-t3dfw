#include "dataLabelMgr.h"

#include "gfxAPI/glad/include/glad/glad.h"

#include "gfxAPI/bufferTexture.h"

//#include "utf8Utils.h"

#include <stdio.h>

dataLabelMgr::dataLabelMgr() 
    //: mStbFont( "./data/fonts/Skinny__.ttf" )
    : mStbFont( "./data/fonts/Spectral-Regular.ttf" )
    , mTtfMeshFont( "./data/fonts/Spectral-Regular.ttf" )
    , mpSdfRoundRectsVbo( nullptr )
    , mpSdfArrowsVbo( nullptr )
    , mSdfRoundRectsBT( {  } )
    , mSdfArrowsBT( {  } ){
    
    printf( "dataLabelMgr ctor\n" );


    printf( "creating SDF shader\n" ); fflush( stdout );
    
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > sdfShaderDesc{
        std::make_pair( "./src/shaders/fullscreenTri.vert.glsl.preprocessed", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/sdf.frag.glsl.preprocessed", GfxAPI::Shader::eShaderStage::FS ), // X-ray of x-y planes
    };
    gfxUtils::createShader( mSdfShader, sdfShaderDesc );
    mSdfShader.use( true );
    mSdfShader.setInt( "u_roundRectsTB", 0 );
    mSdfShader.setInt( "u_arrowsTB", 1 );
    mSdfShader.use( false );

    printf( "creating triangle-fullscreen shader\n" ); fflush( stdout );
    GfxAPI::Shader mFullscreenTriShader;
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > fullscreenTriShaderDesc{
        std::make_pair( "./src/shaders/fullscreenTri.vert.glsl.preprocessed", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/fullscreenTri.frag.glsl.preprocessed", GfxAPI::Shader::eShaderStage::FS ), // X-ray of x-y planes
    };
    gfxUtils::createShader( mFullscreenTriShader, fullscreenTriShaderDesc );
    mFullscreenTriShader.use( true );
    mFullscreenTriShader.setInt( "u_Tex", 0 );
    mFullscreenTriShader.use( false );

    mScreenTriHandle = gfxUtils::createScreenTriGfxBuffers();
}

dataLabelMgr::~dataLabelMgr() {
    printf( "dataLabelMgr dtor\n" );
    //gfxUtils::freeMeshGfxBuffers( mScreenTriHandle );
}

void dataLabelMgr::addLabel( const dataLabel& dataLabel ) {
    mScreenLabels.push_back( dataLabel );

    // fixup text label width to match printed text (that appears centered)
    mScreenLabels.back().roundRect().endPos[0] = mScreenLabels.back().roundRect().startPos[0];
    mScreenLabels.back().roundRect().endPos[0] += 1.25f * mTtfMeshFont.getTextDisplayW( mScreenLabels.back().labelText().c_str() );
    for (auto& arrow : mScreenLabels.back().arrowScreenAttribs()) {
        arrow.startPos[0] = 0.5f * (mScreenLabels.back().roundRect().endPos[0] + mScreenLabels.back().roundRect().startPos[0]);
    }
}

void dataLabelMgr::removeAllLabels() {
    mScreenLabels.clear();
}

void dataLabelMgr::bakeAddedLabels() {
    //GfxAPI::BufferTexture sdfRoundRectsBT{ {} };
    //GfxAPI::BufferTexture sdfArrowsBT{ {} };



#if 1
    { // actually, perform this on each data load when new labels become available!
        mSdfShader.use( true );

        // this data will come from json label file (and will start out in OS, not already NDC-ish as here...
        //const std::vector<sdfRoundRect_t> roundRectDataCPU{
        //    {   .startPos        = { 0.7f, 0.8f },
        //        .endPos          = { 1.1f, 0.8f },
        //        .thickness       = 0.005f,
        //        .cornerRoundness = 0.01f,
        //    },
        //    {   .startPos        = { 0.1f, 0.2f },
        //        .endPos          = { 0.4f, 0.2f },
        //        .thickness       = 0.005f,
        //        .cornerRoundness = 0.01f,
        //    },
        //};
        //const int32_t numSdfRoundRects = static_cast<int32_t>( roundRectDataCPU.size() );

        //const std::vector<sdfArrow_t> arrowDataCPU{
        //    {   .startPos       = { 0.9f,  0.8f  },
        //        .endPos         = { 0.35f, 0.25f },
        //        .shaftThickness = 0.005f,
        //        .headThickness  = 0.005f + 0.01f,
        //    },
        //};
        //const int32_t numSdfArrows = static_cast<int32_t>( arrowDataCPU.size() );

        const int32_t numSdfRoundRects = static_cast<int32_t>( mScreenLabels.size() );
        int32_t numSdfArrows = 0; 
        for (const auto& entry : mScreenLabels) {
            numSdfArrows += entry.numArrows();
        }

        // upload sdf round-rects and arrows used for text labels
        mSdfShader.setIvec2( "u_num_RoundRects_Arrows", { numSdfRoundRects, numSdfArrows } );
        //mSdfShader.setIvec2( "u_num_RoundRects_Arrows", { numSdfRoundRects, 0 } ); //!!!

        mSdfRoundRectsBT.detachVbo();
        /*GfxAPI::Vbo*/ delete mpSdfRoundRectsVbo;
        mpSdfRoundRectsVbo = new GfxAPI::Vbo{ {.numBytes = static_cast<uint32_t>(numSdfRoundRects * sizeof( dataLabel::roundRectAttribs )) } };
        mSdfRoundRectsBT.attachVbo( mpSdfRoundRectsVbo );
        //auto* const pSdfRoundRectsVbo = mSdfRoundRectsVbo.map( GfxAPI::eMapMode::writeOnly );
        uint8_t* const pSdfRoundRectsVbo = reinterpret_cast<uint8_t*>( mpSdfRoundRectsVbo->map( GfxAPI::eMapMode::writeOnly ) );
        //memcpy( pSdfRoundRectsVbo, mScreenLabels .data(), mSdfRoundRectsVbo.desc().numBytes /*roundRectDataCPU.size() * sizeof( sdfRoundRect_t )*/ );
        // bad mem layout!!! instead of one big memcpy, need to perform one memcpy per label
        {
            uint32_t byteOffset = 0;
            uint32_t chunkByteSize = sizeof( dataLabel::roundRectAttribs );
            for (const auto& entry : mScreenLabels) {
                memcpy( pSdfRoundRectsVbo + byteOffset, &entry.roundRect(), chunkByteSize );
                byteOffset += chunkByteSize;
            }
        }
        mpSdfRoundRectsVbo->unmap();

        mSdfArrowsBT.detachVbo();
        delete mpSdfArrowsVbo;
        /*GfxAPI::Vbo*/ mpSdfArrowsVbo = new GfxAPI::Vbo{ {.numBytes = static_cast<uint32_t>( numSdfArrows * sizeof( dataLabel::sdfArrow_t ) ) } };
        mSdfArrowsBT.attachVbo( mpSdfArrowsVbo );
        //auto* const pSdfArrowsVbo = mSdfArrowsVbo.map( GfxAPI::eMapMode::writeOnly );
        uint8_t* const pSdfArrowsVbo = reinterpret_cast<uint8_t*>( mpSdfArrowsVbo->map( GfxAPI::eMapMode::writeOnly ) );
        //memcpy( pSdfArrowsVbo, arrowDataCPU.data(), mSdfArrowsVbo.desc().numBytes );
        {
            uint32_t byteOffset = 0;
            uint32_t chunkByteSize = sizeof( dataLabel::sdfArrow_t );
            for (auto& entry : mScreenLabels) {
                //for (auto& arrowEntry : entry.arrowScreenAttribs()) { // set arrow start pos to label pos
                //    //arrowEntry.startPos = entry.roundRect().startPos;
                //    //arrowEntry.endPos = entry.roundRect().endPos;
                //    arrowEntry = {  .startPos = { 0.9f,  0.8f  },
                //                    .endPos = { 0.35f, 0.25f },
                //                    .shaftThickness = 0.005f,
                //                    .headThickness = 0.005f + 0.01f };
                //}
                memcpy( pSdfArrowsVbo + byteOffset, entry.arrowScreenAttribs().data(), chunkByteSize * entry.arrowScreenAttribs().size() );
                byteOffset += chunkByteSize * entry.arrowScreenAttribs().size();
            }
        }
        mpSdfArrowsVbo->unmap();

        mSdfShader.use( false );
}
#endif

}

void dataLabelMgr::drawScreen( const linAlg::mat4_t& mvpMatrix, const int32_t fbW, const int32_t fbH, uint64_t frameNum ) {

#if 1
    glDisable( GL_DEPTH_TEST );
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );
    glBlendEquation( GL_FUNC_ADD );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

#if 0// update sdf gui-elements draw data
    arrowDataCPU[0].endPos[0] = 0.35f + sinf( frameNum * M_PI / 180.0f );
    auto* const pSdfArrowsVbo = sdfArrowsBT.attachedVbo()->map( GfxAPI::eMapMode::writeOnly );
    memcpy( pSdfArrowsVbo, arrowDataCPU.data(), sdfArrowsBT.attachedVbo()->desc().numBytes );
    sdfArrowsBT.attachedVbo()->unmap();

    roundRectDataCPU[1].startPos[0] = 0.1f + 0.5f * cosf( frameNum * M_PI / 180.0f );
    roundRectDataCPU[1].endPos[0] = 0.4f + 0.5f * cosf( frameNum * M_PI / 180.0f );
    roundRectDataCPU[1].startPos[1] = 0.2f + 0.25f * sinf( frameNum * M_PI / 180.0f );
    roundRectDataCPU[1].endPos[1] = 0.2f + 0.25f * sinf( frameNum * M_PI / 180.0f );
    auto* const pSdfRoundRectsVbo = sdfRoundRectsBT.attachedVbo()->map( GfxAPI::eMapMode::writeOnly );
    memcpy( pSdfRoundRectsVbo, roundRectDataCPU.data(), sdfRoundRectsBT.attachedVbo()->desc().numBytes );
    sdfRoundRectsBT.attachedVbo()->unmap();
#else
    if (!mScreenLabels.empty()){
        //arrowDataCPU[0].endPos[0] = 0.35f + sinf( frameNum * M_PI / 180.0f );
        bool alreadyChanged = false;
        //mSdfArrowsBT.attachVbo( mpSdfArrowsVbo );
        //auto* const pSdfArrowsVbo = mSdfArrowsVbo.map( GfxAPI::eMapMode::writeOnly );
        uint8_t* const pSdfArrowsVbo = reinterpret_cast<uint8_t*>( mpSdfArrowsVbo->map( GfxAPI::eMapMode::writeOnly ) );
        //memcpy( pSdfArrowsVbo, arrowDataCPU.data(), mSdfArrowsVbo.desc().numBytes );
        {
            uint32_t byteOffset = 0;
            uint32_t chunkByteSize = sizeof( dataLabel::sdfArrow_t );
            for (auto& entry : mScreenLabels) {
                if (entry.arrowScreenAttribs().size() > 0 && !alreadyChanged) {
                    entry.arrowScreenAttribs()[0].endPos[0] = 0.35f + sinf( frameNum * M_PI / 180.0f );
                    alreadyChanged = true;
                }
                memcpy( pSdfArrowsVbo + byteOffset, entry.arrowScreenAttribs().data(), chunkByteSize * entry.arrowScreenAttribs().size() );
                byteOffset += chunkByteSize * entry.arrowScreenAttribs().size();
            }
        }
        mpSdfArrowsVbo->unmap();

    }

    if (mScreenLabels.size() >= 2) {
        for (uint32_t i = 1; i <= 2; i++) {
            mScreenLabels[i].roundRect().startPos[0] = 0.1f + 0.5f * cosf( i+frameNum * M_PI / 180.0f );
            mScreenLabels[i].roundRect().endPos[0] = 0.4f + 0.5f * cosf( i+frameNum * M_PI / 180.0f );
            mScreenLabels[i].roundRect().startPos[1] = 0.2f + 0.25f * sinf( i+frameNum * M_PI / 180.0f );
            mScreenLabels[i].roundRect().endPos[1] = 0.2f + 0.25f * sinf( i+frameNum * M_PI / 180.0f );

            mScreenLabels[i].roundRect().endPos[0] = mScreenLabels[i].roundRect().startPos[0];
            mScreenLabels[i].roundRect().endPos[0] += 1.25f * mTtfMeshFont.getTextDisplayW( mScreenLabels[i].labelText().c_str() );


            const float iaspectRatio = static_cast<float>(fbW) / static_cast<float>(fbH);
            const float ixAspect = (iaspectRatio > 1.0f) ? iaspectRatio : 1.0f;
            const float iyAspect = (iaspectRatio > 1.0f) ? 1.0f : 1.0f / iaspectRatio;

            uint32_t idx = 0;
            for (auto& arrow : mScreenLabels[i].arrowScreenAttribs()) {
                arrow.startPos[0] = 0.5f * (mScreenLabels[i].roundRect().endPos[0] + mScreenLabels[i].roundRect().startPos[0]);
                arrow.startPos[1] = 0.5f * (mScreenLabels[i].roundRect().endPos[1] + mScreenLabels[i].roundRect().startPos[1]);

                //arrow.endPos[0] = arrow.startPos[0] + 0.2f;
                //arrow.endPos[1] = arrow.startPos[1] + 0.2f;

                linAlg::vec4_t pos{ mScreenLabels[i].arrowDataPos3D()[idx][0], mScreenLabels[i].arrowDataPos3D()[idx][1], mScreenLabels[i].arrowDataPos3D()[idx][2], 1.0f };
                linAlg::mat4_t trafoMat = mvpMatrix;
                //linAlg::inverse( trafoMat, mvpMatrix );
                linAlg::applyTransformationToPoint( trafoMat, &pos, 1 );
                arrow.endPos[0] = (pos[0] / pos[3]) * ixAspect;
                arrow.endPos[1] = (pos[1] / pos[3]) * iyAspect;

                idx++;
            }
        }



        uint8_t* const pSdfRoundRectsVbo = reinterpret_cast<uint8_t*>(mpSdfRoundRectsVbo->map( GfxAPI::eMapMode::writeOnly ));
        //memcpy( pSdfRoundRectsVbo, mScreenLabels .data(), mSdfRoundRectsVbo.desc().numBytes /*roundRectDataCPU.size() * sizeof( sdfRoundRect_t )*/ );
        // bad mem layout!!! instead of one big memcpy, need to perform one memcpy per label
        {
            uint32_t byteOffset = 0;
            uint32_t chunkByteSize = sizeof( dataLabel::roundRectAttribs );
            for (const auto& entry : mScreenLabels) {
                memcpy( pSdfRoundRectsVbo + byteOffset, &entry.roundRect(), chunkByteSize );
                byteOffset += chunkByteSize;
            }
        }
        mpSdfRoundRectsVbo->unmap();

        uint8_t* const pSdfArrowsVbo = reinterpret_cast<uint8_t*>( mpSdfArrowsVbo->map( GfxAPI::eMapMode::writeOnly ) );
        {
            uint32_t byteOffset = 0;
            uint32_t chunkByteSize = sizeof( dataLabel::sdfArrow_t );
            for (auto& entry : mScreenLabels) {
                memcpy( pSdfArrowsVbo + byteOffset, entry.arrowScreenAttribs().data(), chunkByteSize * entry.arrowScreenAttribs().size() );
                byteOffset += chunkByteSize * entry.arrowScreenAttribs().size();
            }
        }
        mpSdfArrowsVbo->unmap();

    }

#endif


    mSdfRoundRectsBT.bindToTexUnit( 0 );
    mSdfArrowsBT.bindToTexUnit( 1 );
    mSdfShader.use( true );
    {
        const float aspectRatio = static_cast<float>(fbW) / static_cast<float>(fbH);
        const float xAspect = (aspectRatio > 1.0f) ? aspectRatio : 1.0f;
        const float yAspect = (aspectRatio > 1.0f) ? 1.0f : 1.0f / aspectRatio;
        mSdfShader.setVec2( "u_scaleRatio", {xAspect, yAspect} );
    }
    glBindVertexArray( mScreenTriHandle.vaoHandle );
    glDrawArrays( GL_TRIANGLES, 0, 3 );
    mSdfShader.use( false );
    mSdfRoundRectsBT.unbindFromTexUnit();
    mSdfArrowsBT.unbindFromTexUnit();
#endif

#if 0 // works fine, but settling for ttf2mesh font
    {
        const float aspectRatio = static_cast<float>(fbW) / static_cast<float>(fbH);
        const float xAspect = (aspectRatio > 1.0f) ? aspectRatio : 1.0f;
        const float yAspect = (aspectRatio > 1.0f) ? 1.0f : 1.0f / aspectRatio;
        //mStbFont.setAspectRatios( { xAspect, yAspect } );
        mStbFont.setAspectRatios( { 1.0f, 1.0f } );
    }

    constexpr linAlg::vec4_t fontColor{ 0.3f, 0.9f, 0.7f, 0.5f };
    mStbFont.renderText( -512.0f, -512.0f + 24.0f, fontColor, "Left Top" );
    mStbFont.renderText( -512.0f, -512.0f + 24.0f + 32.0f, fontColor, "Left Top Drunter" );
    mStbFont.renderText( -512.0f,  512.0f -  4.0f, fontColor, "Left Btm" );
    mStbFont.renderText( -512.0f,  512.0f -  4.0f - 32.0f, fontColor, "Left Btm Drueber" );
    mStbFont.renderText(  0.0f,  0.0f, fontColor, "Hello There" );

    mStbFont.renderText(  512.0f - 8.0f * 16.0f, -512.0f + 24.0f, fontColor, "Right Top" );
    mStbFont.renderText(  512.0f - 8.0f * 16.0f,  512.0f -  4.0f, fontColor, "Right Btm" );
#endif

#if 1
    {
        const float aspectRatio = static_cast<float>(fbW) / static_cast<float>(fbH);
        const float xAspect = (aspectRatio > 1.0f) ? aspectRatio : 1.0f;
        const float yAspect = (aspectRatio > 1.0f) ? 1.0f : 1.0f / aspectRatio;
        mTtfMeshFont.setAspectRatios( { xAspect, yAspect } );


        glEnable( GL_DEPTH_TEST );
        //glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );

        glDisable( GL_BLEND );
        glDisable( GL_CULL_FACE );
        constexpr linAlg::vec4_t fontColor2d{ 0.1f, 0.3f, 0.9f, 0.5f };
        //mTtfMeshFont.renderText2d( +5.0f, 2.0f, fontColor2d, _TEXT( "TTF Font 2D, 123" ) );
        //mTtfMeshFont.renderText2d( 0.0f, 0.0f, fontColor2d, _TEXT( "TTF Font 2D, 123" ) );
        //mTtfMeshFont.renderText2d( -0.8f, 0.0f, _TEXT( "A" ) );

        //const std::vector<sdfRoundRect_t> roundRectDataCPU{
        //    {   .startPos        = { 0.7f, 0.8f },
        //    .endPos          = { 1.1f, 0.8f },
        //    .thickness       = 0.005f,
        //    .cornerRoundness = 0.01f,
        //    },

        mTtfMeshFont.setRelFontSize( xAspect / 40.0f );

        const float startFactor = 0.1f;
        const float downOffsetFactor = 1.75f;
        //mTtfMeshFont.renderText2d( ( 1.0f - startFactor ) * 0.7f + startFactor * 1.1f, 0.8f - ( 0.1f * (0.5f * 0.005f) * 32.0f ), fontColor2d, _TEXT( "Font 2D Widget" ) );
        //for (const auto& entry : roundRectDataCPU) {
        for (const auto& entry : mScreenLabels) {

            const auto& text = entry.labelText();

            //const float textW = mTtfMeshFont.getTextDisplayW( _TEXT( "Font 2D Widget" ) ); // actually the widget should set its width from the contained text
            const float textW = mTtfMeshFont.getTextDisplayW( text.c_str() );
            //const float textH = mTtfMeshFont.getTextDisplayH( _TEXT( "Font 2D Widget" ) ); // actually the widget should set its height from the contained text
            
            //const linAlg::vec2_t textMinMaxY = mTtfMeshFont.getTextDisplayMinMaxY( _TEXT( "Font 2D Widget" ) ); // actually the widget should set its height from the contained text
            const linAlg::vec2_t textMinMaxY = mTtfMeshFont.getTextDisplayMinMaxY( text.c_str() ); // actually the widget should set its height from the contained text

            //const float fontStartX = (1.0f - startFactor) * entry.startPos[0] + startFactor * entry.endPos[0];

            const float midXWidget = 0.5f * (entry.roundRect().startPos[0] + entry.roundRect().endPos[0]);
            const float fontStartX = midXWidget - 0.5f * textW;

            //const float fontStartY = entry.startPos[1] - (downOffsetFactor * (entry.thickness) ) /*  / yAspect */;
            const float fontStartY = entry.roundRect().startPos[1] - 0.5f * (textMinMaxY[1]+textMinMaxY[0]); // -0.5f * mTtfMeshFont.getRelFontSize();

            //mTtfMeshFont.renderText2d( fontStartX, fontStartY, fontColor2d, _TEXT( "Font 2D Widget" ) );
            mTtfMeshFont.renderText2d( fontStartX, fontStartY, fontColor2d, text.c_str() );
        }

        //constexpr linAlg::vec4_t fontColor3d{ 0.9f, 0.2f, 0.1f, 0.5f };
        ////mTtfMeshFont.renderText3d( 0.8f, 0.0f, 0.0f, fontColor3d, _TEXT( "TTF Font 3D, 456" ) );
        //mTtfMeshFont.renderText3d( -3.0f, -8.0f, 0.0f, fontColor3d, _TEXT( "TTF Font 3D, 456" ) );
        ////mTtfMeshFont.renderText3d( 0.0f, 0.0f, _TEXT( "B" ) );
    }
#endif

}