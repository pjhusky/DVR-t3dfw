#include "dataLabelMgr.h"

#include "gfxAPI/glad/include/glad/glad.h"

#include "gfxAPI/bufferTexture.h"

//#include "utf8Utils.h"

#include <stdio.h>

namespace {
    static float sdOrientedBox( const linAlg::vec2_t& p, const linAlg::vec2_t& a, const linAlg::vec2_t& b, const float th ) {
        const float l = linAlg::dist( a, b );// length( b - a );
        const linAlg::vec2_t d = (b-a)/l;
        linAlg::vec2_t q = p-(a+b)*0.5f;
        
        linAlg::mat2_t rotMat;
        rotMat[0] = {  d[0], d[1] };
        rotMat[1] = { -d[1], d[0] };

        q = rotMat*q;

        linAlg::sub( q, linAlg::vec2_t{ fabsf( q[0] ), fabsf( q[1] ) }, linAlg::vec2_t{ l * 0.5f, th } );
        
        //return length(max(q,0.0)) + min(max(q.x,q.y),0.0);    
        linAlg::vec2_t max_q_0{ std::max( q[0],0.0f ), std::max( q[1],0.0f ) };
        return linAlg::len( max_q_0 ) + std::min( std::max( q[0], q[1] ), 0.0f );
    }

}

dataLabelMgr::dataLabelMgr() 
    //: mStbFont( "./data/fonts/Skinny__.ttf" )
    : mStbFont( "./data/fonts/Spectral-Regular.ttf" )
    , mTtfMeshFont( "./data/fonts/Spectral-Regular.ttf" )
    , mpSdfRoundRectsVbo( nullptr )
    , mpSdfArrowsVbo( nullptr )
    , mSdfRoundRectsBT( {  } )
    , mSdfArrowsBT( {  } ) {
    
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

void dataLabelMgr::simulateForcesForLabelPlacement() {
    const int32_t numSdfRoundRects = static_cast<int32_t>( mScreenLabels.size() );
    
    // copy round Rect
    std::vector< dataLabel::roundRectAttribs > currRoundRects;
    currRoundRects.resize( numSdfRoundRects );
    for (int i = 0; i < numSdfRoundRects; i++) { // round rects
        currRoundRects[i] = mScreenLabels[i].roundRect();
    }

    float d = 1000000.0f;
    float d_px = 1000000.0f;
    float d_mx = 1000000.0f;
    float d_py = 1000000.0f;
    float d_my = 1000000.0f;
//#pragma omp parallel for /*collapse(2)*/ schedule(dynamic, 1)		// OpenMP <== must sync on d updates!!!
//#pragma omp parallel for reduction(min:d)
    for ( int i = 0;  i < numSdfRoundRects; i++ ) { // round rects
        const auto& srcRoundRect = currRoundRects[i];
        const auto srcRoundRect_center = (srcRoundRect.startPos + srcRoundRect.endPos) * 0.5f;

        //const auto srcRoundRect_center_pxpy = srcRoundRect_center + linAlg::vec2_t{ 0.001f,  0.001f};
        //const auto srcRoundRect_center_mxpy = srcRoundRect_center + linAlg::vec2_t{-0.001f,  0.001f};
        //const auto srcRoundRect_center_pxmy = srcRoundRect_center + linAlg::vec2_t{ 0.001f, -0.001f};
        //const auto srcRoundRect_center_mxmy = srcRoundRect_center + linAlg::vec2_t{-0.001f, -0.001f};
        const auto srcRoundRect_center_px = srcRoundRect_center + linAlg::vec2_t{ 0.00001f,  0.0f };
        const auto srcRoundRect_center_mx = srcRoundRect_center + linAlg::vec2_t{-0.00001f,  0.0f };
        const auto srcRoundRect_center_py = srcRoundRect_center + linAlg::vec2_t{ 0.0f,    0.00001f};
        const auto srcRoundRect_center_my = srcRoundRect_center + linAlg::vec2_t{ 0.0f,   -0.00001f};

        linAlg::vec2_t deltaVec{ 0.0f, 0.0f};
        const float deltaFactor = 0.000001f;

        for (int j = 0; j < numSdfRoundRects; j++) { // round rects
            if (i == j) { continue; }
            const auto& otherRoundRect = currRoundRects[j];

            const auto& startPos = otherRoundRect.startPos;
            const auto& endPos = otherRoundRect.endPos;
            const float thickness = otherRoundRect.thickness;

            const auto otherCenter = 0.5f * (startPos + endPos);

            const auto centerDelta = otherCenter - srcRoundRect_center;
            deltaVec = deltaVec + linAlg::vec2_t{ deltaFactor / (centerDelta[0] * centerDelta[0] + 0.0001f), deltaFactor / (centerDelta[1]*centerDelta[1] + 0.0001f) };

            //float roundness = srcRoundRect.cornerRoundness;
            //float currDist_center = sdOrientedBox( srcRoundRect_center, startPos, endPos, thickness );
            //float currDist_px = sdOrientedBox( srcRoundRect_center_px, startPos, endPos, thickness );
            //float currDist_mx = sdOrientedBox( srcRoundRect_center_mx, startPos, endPos, thickness );
            //float currDist_py = sdOrientedBox( srcRoundRect_center_py, startPos, endPos, thickness );
            //float currDist_my = sdOrientedBox( srcRoundRect_center_my, startPos, endPos, thickness );

            //float currDist_px = linAlg::dist( srcRoundRect_center_px, 0.5f * ( startPos + endPos ) );
            //float currDist_mx = linAlg::dist( srcRoundRect_center_mx, 0.5f * ( startPos + endPos ) );
            //float currDist_py = linAlg::dist( srcRoundRect_center_py, 0.5f * ( startPos + endPos ) );
            //float currDist_my = linAlg::dist( srcRoundRect_center_my, 0.5f * ( startPos + endPos ) );


            //d = std::min( d, currDist );
            //d_px = std::min( d_px, currDist_px );
            //d_mx = std::min( d_mx, currDist_mx );
            //d_py = std::min( d_py, currDist_py );
            //d_my = std::min( d_my, currDist_my );
            //d_px += currDist_px;
            //d_mx += currDist_mx;
            //d_py += currDist_py;
            //d_my += currDist_my;
            //d_px += std::max( 0.0f, currDist_px );
            //d_mx += std::max( 0.0f, currDist_mx );
            //d_py += std::max( 0.0f, currDist_py );
            //d_my += std::max( 0.0f, currDist_my );

            //d_px = std::min( d_px, std::max( currDist_px, 0.0f ) );
            //d_mx = std::min( d_mx, std::max( currDist_mx, 0.0f ) );
            //d_py = std::min( d_py, std::max( currDist_py, 0.0f ) );
            //d_my = std::min( d_my, std::max( currDist_my, 0.0f ) );

            //d_px = 1.0f - std::min( d_px, currDist_px );
            //d_mx = 1.0f - std::min( d_mx, currDist_mx );
            //d_py = 1.0f - std::min( d_py, currDist_py );
            //d_my = 1.0f - std::min( d_my, currDist_my );
        }

        //d_px = linAlg::clamp( d_px * d_px, 0.0f, 1.0f );
        //d_mx = linAlg::clamp( d_mx * d_mx, 0.0f, 1.0f );
        //d_py = linAlg::clamp( d_py * d_py, 0.0f, 1.0f );
        //d_my = linAlg::clamp( d_my * d_my, 0.0f, 1.0f );
        //d_px = 1.0f / ( (d_px * d_px) + 0.00000001f );
        //d_mx = 1.0f / ( (d_mx * d_mx) + 0.00000001f );
        //d_py = 1.0f / ( (d_py * d_py) + 0.00000001f );
        //d_my = 1.0f / ( (d_my * d_my) + 0.00000001f );

        const float recipCount = 1.0f / numSdfRoundRects;
        //d_px *= recipCount;
        //d_mx *= recipCount;
        //d_py *= recipCount;
        //d_my *= recipCount;

        //d_px = 1.0f / ( (d_px * d_px) + 0.00000001f );
        //d_mx = 1.0f / ( (d_mx * d_mx) + 0.00000001f );
        //d_py = 1.0f / ( (d_py * d_py) + 0.00000001f );
        //d_my = 1.0f / ( (d_my * d_my) + 0.00000001f );

        //// now move point along the negative (approximated) gradient
        auto& dstRoundRect = mScreenLabels[i].roundRect();
        //const float dt = 0.001f;
        //const float dx =     dt * (d_px - d_mx)    + (rand() % 101 - 50) * 0.00001f;
        //const float dy =     dt * (d_py - d_my)    + (rand() % 101 - 50) * 0.00001f;

        const float dx = deltaVec[0] * recipCount;
        const float dy = deltaVec[1] * recipCount;
        dstRoundRect.startPos[0] -= dx;
        dstRoundRect.startPos[1] -= dy;

        //dstRoundRect.endPos   = dstRoundRect.endPos - dt * (d_px - d_mx) + ( rand() % 101 ) * 0.00005f;
        dstRoundRect.endPos = dstRoundRect.endPos - linAlg::vec2_t{dx, dy};

    }

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
#elif 1

#if 0 // just some debug animations
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
    #else // actual position updates


    for (uint32_t i = 0; i < mScreenLabels.size(); i++) {


        //mScreenLabels[i].roundRect().startPos[0] = 0.1f + 0.5f * cosf( i+frameNum * M_PI / 180.0f );
        //mScreenLabels[i].roundRect().endPos[0] = 0.4f + 0.5f * cosf( i+frameNum * M_PI / 180.0f );
        //mScreenLabels[i].roundRect().startPos[1] = 0.2f + 0.25f * sinf( i+frameNum * M_PI / 180.0f );
        //mScreenLabels[i].roundRect().endPos[1] = 0.2f + 0.25f * sinf( i+frameNum * M_PI / 180.0f );

        //mScreenLabels[i].roundRect().endPos[0] = mScreenLabels[i].roundRect().startPos[0];
        //mScreenLabels[i].roundRect().endPos[0] += 1.25f * mTtfMeshFont.getTextDisplayW( mScreenLabels[i].labelText().c_str() );


        const float iaspectRatio = static_cast<float>(fbW) / static_cast<float>(fbH);
        const float ixAspect = (iaspectRatio > 1.0f) ? iaspectRatio : 1.0f;
        const float iyAspect = (iaspectRatio > 1.0f) ? 1.0f : 1.0f / iaspectRatio;

        uint32_t idx = 0;
        for (auto& arrow : mScreenLabels[i].arrowScreenAttribs()) {
            arrow.startPos[0] = 0.5f * (mScreenLabels[i].roundRect().endPos[0] + mScreenLabels[i].roundRect().startPos[0]);
            arrow.startPos[1] = 0.5f * (mScreenLabels[i].roundRect().endPos[1] + mScreenLabels[i].roundRect().startPos[1]);

            linAlg::vec4_t pos{ mScreenLabels[i].arrowDataPos3D()[idx][0], mScreenLabels[i].arrowDataPos3D()[idx][1], mScreenLabels[i].arrowDataPos3D()[idx][2], 1.0f };
            
            linAlg::applyTransformationToPoint( mvpMatrix, &pos, 1 );
            arrow.endPos[0] = (pos[0] / pos[3]) * ixAspect;
            arrow.endPos[1] = (pos[1] / pos[3]) * iyAspect;

            idx++;
        }



        {

            const linAlg::vec4_t volDataCenterPosOS{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t volDataCenterPosNDC = volDataCenterPosOS;
            linAlg::applyTransformationToPoint( mvpMatrix, &volDataCenterPosNDC, 1 );

            volDataCenterPosNDC[0] = (volDataCenterPosNDC[0] / volDataCenterPosNDC[3]) * ixAspect;
            volDataCenterPosNDC[1] = (volDataCenterPosNDC[1] / volDataCenterPosNDC[3]) * iyAspect;

            const int32_t numSdfRoundRects = static_cast<int32_t>(mScreenLabels.size());
            for (int i = 0; i < numSdfRoundRects; i++) { // round rects
                auto& currRect = mScreenLabels[i].roundRect();
                //const auto currRectCenter = 0.5f * (currRect.startPos + currRect.endPos);

                linAlg::vec2_t arrowEndPosAccum{ 0.0f, 0.0f };
                for (const auto arrow : mScreenLabels[i].arrowScreenAttribs()) {
                    arrowEndPosAccum = arrowEndPosAccum + arrow.endPos;
                }
                arrowEndPosAccum = arrowEndPosAccum * (1.0f / mScreenLabels[i].arrowScreenAttribs().size());
                auto diffVec = arrowEndPosAccum - linAlg::vec2_t{ volDataCenterPosNDC[0], volDataCenterPosNDC[1] };
                if (linAlg::len( diffVec ) < 0.000001f) { diffVec = { 1.0f, 0.0f }; }

                //linAlg::vec3_t arrowEndPosAccum{ 0.0f, 0.0f, 0.0f };
                //for (const auto arrowEndPos : mScreenLabels[i].arrowDataPos3D()) {
                //    arrowEndPosAccum = arrowEndPosAccum + arrowEndPos;
                //}
                //arrowEndPosAccum = arrowEndPosAccum * (1.0f / mScreenLabels[i].arrowDataPos3D().size());
                //auto diffVec = arrowEndPosAccum - linAlg::vec3_t{ volDataCenterPosOS[0], volDataCenterPosOS[1], volDataCenterPosOS[2] };



                //diffVec = linAlg::normalizeRet( diffVec ) * (0.2f * (i+1));
                diffVec = linAlg::normalizeRet( diffVec ) * 0.75f;


                const auto rectDiffVec = currRect.endPos - currRect.startPos;
                currRect.startPos = linAlg::vec2_t{ volDataCenterPosNDC[0], volDataCenterPosNDC[1] } + diffVec;
                currRect.endPos = currRect.startPos + rectDiffVec;

                // extend away along this dir
            }

            //simulateForcesForLabelPlacement();

            
        }




    #endif


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