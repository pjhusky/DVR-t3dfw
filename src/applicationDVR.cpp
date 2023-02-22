// ### TODO ###

// * more than 1 Levoy iso surface?
// * per-surface material (diffuse|specular) settings => make skin iso-surface more diffuse, and bone iso-surface more specular 
// * printVec raus-refaktoren
// * # ON-GOING # ... auch merge der beiden shader (box exit pos vs. calc exit pos) shader make more use of common functions - refactoring
// * SharedMemIPC sharedMem{ "DVR_shared_memory" }; ==> define the name "DVR_shared_memory" in one location, accessible to each file that needs to access the shared mem


// schaff ich jetzt NICHT!!!
// - empty space skipping
//      set up additive blending in the framebuffer
//      render small boxes front2back - immer alle small boxes, ABER jede dieser boxen repräsentiert ein texel einer groben 3D texture die das max der transparency-mapped densities ist
//      kann schon im VS mit einem single tex access die gesamte box "offscreen" lenken bzw. collapsen lassen ==> FS wird gar nicht erst invoked
//
// - CSG operations (either permanent within the 3D texture, or temporary as in a stencil trick)


// debug vis empty space skipping
// GUI toggle switch empty space skipping


// * Documentation:
// early-ray termination, weil front-2-back compositing
// empty-space skipping "light" => only sample volume not empty space around it, but no more hierarchies "inside" the volume
// TF load-save als convenience feature
// jittered sampling mit TC - macht außerdem smooth TF transitions beim TF-laden auch smooth gradient-calc switch
// UI renders in lock-step with renderer - not ideal but easier to manage with ImGui...
// shader recompilation (with glslValidator preprocessing) working, but locked to Windows atm
//      - NOTE-TO-SELF: launch glslValidator.exe with "cmd /C ..." so that '>' piping of stdout to file works 
// bei SW arch kann ich das mit zwei prozessen für 2 opengl windows anmerken und daß sie über shared mem kommunizieren (quasi named pipes?)
//      - volume data preproc mittels OpenMP thread-parallel

#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/rbo.h"
#include "gfxAPI/fbo.h"
#include "gfxAPI/ubo.h"
#include "gfxAPI/checkErrorGL.h"


#include "applicationDVR_common.h"

#include "applicationDVR.h"

#include "fileLoaders/volumeData.h"
#include "fileLoaders/stlModel.h" // used for the unit-cube

#include "arcBall/arcBallControls.h"


#include <math.h>
#include <float.h>
#include <iostream>
#include <tchar.h>
#include <filesystem>

#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include <algorithm>
#include <execution>

#include <cassert>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "src/shaders/dvrCommonDefines.h.glsl"

//#include <concurrent_vector.h>
#include <omp.h>

using namespace ArcBall;

namespace {

    struct brickSortData_t {
        int32_t x, y, z;
        //float distToNearPlane;
        //float distFromViewRay;
    };
    static std::vector<brickSortData_t> visibleBricksWithDists;
    static std::mutex visibleBricksWithDists_mutex;
    
    //static Concurrency::concurrent_vector<brickSortData_t> visibleBricksWithDists;

    static std::vector< std::vector< brickSortData_t > > threadBsd; 


    static DVR_GUI::eRayMarchAlgo rayMarchAlgo = DVR_GUI::eRayMarchAlgo::fullscreenBoxIsect;
    //static DVR_GUI::eRayMarchAlgo rayMarchAlgo = DVR_GUI::eRayMarchAlgo::backfaceCubeRaster;

    static DVR_GUI::eVisAlgo      visAlgo      = DVR_GUI::eVisAlgo::levoyIsosurface;

    static constexpr int32_t brickLen = static_cast<int32_t>( BRICK_BLOCK_DIM );
    static constexpr linAlg::i32vec3_t volBrickDim{ brickLen, brickLen, brickLen };

    constexpr static float fovY_deg = 60.0f;

    constexpr float angleDamping = 0.85f;
    constexpr float panDamping = 0.25f;
    constexpr float mouseSensitivity = 0.23f;
    constexpr float zoomSpeedBase = 1.5f;
    float zoomSpeed = zoomSpeedBase;

    static linAlg::vec2_t surfaceIsoAndThickness{ 0.5f, 8.0f / 4096.0f };

    static linAlg::vec3_t pivotCompensationES{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t rotPivotOffset{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t rotPivotPosOS{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t rotPivotPosWS{ 0.0f, 0.0f, 0.0f };

    //static linAlg::vec4_t prevSphereCenterES{ 0.0f, 0.0f, 0.0f, 1.0f };
    static linAlg::vec3_t prevRefPtES{ 0.0f, 0.0f, 0.0f };

    linAlg::mat3x4_t prevModelMatrix3x4;
    linAlg::mat3x4_t prevViewMatrix3x4;

    constexpr static float initialCamZoomDist = 2.75f;
    constexpr static float initialCamZoomDistTestSuite = 2.0f;
    static float camZoomDist = 0.0f;
    static float targetCamZoomDist = initialCamZoomDist;

    static linAlg::vec3_t targetPanDeltaVector{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t panVector{ 0.0f, 0.0f, 0.0f };

    static constexpr float camSpeedFact = 1.0f; // should be a certain fraction of the bounding sphere radius
    static float frameDelta = 0.016f; // TODO: actually calculate frame duration in the main loop

    static constexpr linAlg::vec4_t greenScreenColor{ 0.0f, 1.0f, 1.0f, 1.0f };


    static bool pressedOrRepeat( GLFWwindow *const pWindow, const int32_t key ) {
        const auto keyStatus = glfwGetKey( pWindow, key );
        return ( keyStatus == GLFW_PRESS || keyStatus == GLFW_REPEAT );
    }
    
    static void printVec( const std::string& label, const linAlg::vec3_t& vec ) {
        printf( "%s: (%f|%f|%f)\n", label.c_str(), vec[0], vec[1], vec[2] );
    }
    static void printVec( const std::string& label, const linAlg::vec4_t& vec ) {
        printf( "%s: (%f|%f|%f|%f)\n", label.c_str(), vec[0], vec[1], vec[2], vec[3] );
    }
    
    static void processInput( GLFWwindow *const pWindow ) {
        if ( glfwGetKey( pWindow, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) { glfwSetWindowShouldClose( pWindow, true ); }

        float camSpeed = camSpeedFact;
        if ( pressedOrRepeat( pWindow, GLFW_KEY_LEFT_SHIFT ) )  { camSpeed *= 5.0f; zoomSpeed *= 4.0f; }
        if ( pressedOrRepeat( pWindow, GLFW_KEY_RIGHT_SHIFT ) ) { camSpeed *= 0.1f; zoomSpeed *= 0.25f; }

        if (pressedOrRepeat( pWindow, GLFW_KEY_W ))    { targetCamZoomDist -= camSpeed * frameDelta; }
        if (pressedOrRepeat( pWindow, GLFW_KEY_S ))    { targetCamZoomDist += camSpeed * frameDelta; }

        // if (pressedOrRepeat( pWindow, GLFW_KEY_LEFT )) { key_dx -= 10.0f * camSpeed * frameDelta; }
        // if (pressedOrRepeat( pWindow, GLFW_KEY_RIGHT)) { key_dx += 10.0f * camSpeed * frameDelta; }

        if (pressedOrRepeat( pWindow, GLFW_KEY_R )) { targetPanDeltaVector[1] -= camSpeed * frameDelta; }
        if (pressedOrRepeat( pWindow, GLFW_KEY_F )) { targetPanDeltaVector[1] += camSpeed * frameDelta; }

        if (pressedOrRepeat( pWindow, GLFW_KEY_A )) { targetPanDeltaVector[0] += camSpeed * frameDelta; }
        if (pressedOrRepeat( pWindow, GLFW_KEY_D )) { targetPanDeltaVector[0] -= camSpeed * frameDelta; }

        constexpr static float pivotSpeed = 4.0f;
        if (pressedOrRepeat( pWindow, GLFW_KEY_KP_6 )) { rotPivotOffset[0] += camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }
        if (pressedOrRepeat( pWindow, GLFW_KEY_KP_4 )) { rotPivotOffset[0] -= camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }

        if (pressedOrRepeat( pWindow, GLFW_KEY_KP_9 )) { rotPivotOffset[1] += camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }
        if (pressedOrRepeat( pWindow, GLFW_KEY_KP_3 )) { rotPivotOffset[1] -= camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }

        if (pressedOrRepeat( pWindow, GLFW_KEY_KP_8 )) { rotPivotOffset[2] += camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }
        if (pressedOrRepeat( pWindow, GLFW_KEY_KP_2 )) { rotPivotOffset[2] -= camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }

        // if (pressedOrRepeat( pWindow, GLFW_KEY_O )) { testTiltRadAngle += camSpeed * frameDelta * pivotSpeed; }
        // if (pressedOrRepeat( pWindow, GLFW_KEY_P )) { testTiltRadAngle -= camSpeed * frameDelta * pivotSpeed; }
    }


    static float mouseWheelOffset = 0.0f;
    static void mouseWheelCallback( GLFWwindow* window, double xoffset, double yoffset ) {
        mouseWheelOffset = static_cast<float>(yoffset);
    }
    

#define STRIP_INCLUDES_FROM_INL 1
#include "applicationInterface/applicationGfxHelpers.inl"

    static std::vector< uint32_t > fbos{ 0 };
    static GfxAPI::Texture colorRenderTargetTex;
    static GfxAPI::Texture silhouetteRenderTargetTex;
    static GfxAPI::Texture normalRenderTargetTex;
    static GfxAPI::Texture depthRenderTargetTex;

#if 1
    static void framebufferSizeCallback( GLFWwindow* pWindow, int width, int height ) {

        printf( "new window size %d x %d\n", width, height );

        glCheckError();

        // resize render targets here, like so:
        //destroyRenderTargetTextures( fbos, colorRenderTargetTex, normalRenderTargetTex, silhouetteRenderTargetTex, depthRenderTargetTex );
        //createRenderTargetTextures( renderTargetW, renderTargetH, fbos, colorRenderTargetTex, normalRenderTargetTex, silhouetteRenderTargetTex, depthRenderTargetTex );

        glCheckError();
    }
#endif

    static void calculateProjectionMatrix( const int32_t fbWidth, const int32_t fbHeight, linAlg::mat4_t& projMatrix ) {

        const float ratio = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

        float l = -0.5f, r = +0.5f;
        float b = -0.5f, t = +0.5f;

        if (ratio > 1.0f) { l *= ratio; r *= ratio; }
        else if (ratio < 1.0f) { b /= ratio; t /= ratio; }

        constexpr float n = +1.0f, f = +1000.0f;
        linAlg::loadPerspectiveMatrix( projMatrix, l, r, b, t, n, f );
    }

    static void calculateFovYProjectionMatrix( const int32_t fbWidth, const int32_t fbHeight, const float fovY_deg, linAlg::mat4_t& projMatrix ) {

        const float ratio = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

        constexpr float n = +0.2f;
        constexpr float f = +1000.0f;

        linAlg::loadPerspectiveFovYMatrix( projMatrix, fovY_deg, ratio, n, f );
    }

    
} // namespace


extern "C" {
    static void atExit() { 
        SharedMemIPC( ApplicationDVR_common::sharedMemId ).put( "stopTransferFunctionApp", "true" ); 
    }
}

ApplicationDVR::ApplicationDVR(
    const GfxAPI::ContextOpenGL& contextOpenGL )
    : mContextOpenGL( contextOpenGL ) 
    , mDataFileUrl( "" )
    , mpData( nullptr )
    , mpDensityTex3d( nullptr )
    , mpDensityLoResTex3d( nullptr )
    , mpEmptySpaceTex2d( nullptr )
    , mpGradientTex3d( nullptr )
    , mpDensityColorsTex2d( nullptr )
#if 0
    , mpGuiTex( nullptr )
    , mpGuiFbo( nullptr )
#endif
    , mpVol_RT_Tex( nullptr )
    , mpVol_RT_Rbo( nullptr )
    , mpVol_RT_Fbo( nullptr )
    , mpLight_Ubo( nullptr )
    , mpTFProcess( nullptr )
    , mpSCProcess( nullptr )
    , mSharedMem( ApplicationDVR_common::sharedMemId )
    , mGrabCursor( true ) {

    std::atexit( atExit );

    mVolLoResData.clear();

    mSharedMem.put( "histoBucketEntries", std::to_string( VolumeData::mNumHistogramBuckets ) );
    DVR_GUI::InitGui( contextOpenGL );

    mScreenTriHandle = gfxUtils::createScreenTriGfxBuffers();
    
    const GfxAPI::Ubo::Desc lightUboDesc{
        .numBytes = sizeof(DVR_GUI::LightParameters),
    };
    mpLight_Ubo = new GfxAPI::Ubo( lightUboDesc );

    mSharedMem.put( "DVR_WatchdogTime_shouldRun", "true" );
    mpWatchdogThread = new std::thread( [=, this]() {
        while (mSharedMem.get( "DVR_WatchdogTime_shouldRun" ) == "true") {
            mSharedMem.put( "DVR_TF_tick", "1" );
            std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        }
    } );
    mSharedMem.put( "loadTF", " " );
    mSharedMem.put( "saveTF", " " );
    

    GfxAPI::Texture::Desc_t emptySpaceTexDesc{
        .texDim = {1024,1024,0},
        .numChannels = 1,
        .channelType = GfxAPI::eChannelType::u8,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    const uint32_t mipLvl = 0;

    delete mpEmptySpaceTex2d;
    mpEmptySpaceTex2d = new GfxAPI::Texture;
    mpEmptySpaceTex2d->create( emptySpaceTexDesc );
    mpEmptySpaceTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToEdge, 0 );
    mpEmptySpaceTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToEdge, 1 );
    //mpEmptySpaceTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 2 );
    const auto minFilter = GfxAPI::eFilterMode::box;
    const auto magFilter = GfxAPI::eFilterMode::box;
    const auto mipFilter = GfxAPI::eFilterMode::box;
    mpEmptySpaceTex2d->setFilterMode( minFilter, magFilter, mipFilter );

}

ApplicationDVR::~ApplicationDVR() {
    delete mpData;
    mpData = nullptr;
    
    delete mpDensityTex3d;
    mpDensityTex3d = nullptr;

    delete mpDensityLoResTex3d;
    mpDensityLoResTex3d = nullptr;

    delete mpEmptySpaceTex2d;
    mpEmptySpaceTex2d = nullptr;

    delete mpGradientTex3d;
    mpGradientTex3d = nullptr;
    
    delete mpDensityColorsTex2d;
    mpDensityColorsTex2d = nullptr;

#if 0
    delete mpGuiTex;
    mpGuiTex = nullptr;

    delete mpGuiFbo;
    mpGuiFbo = nullptr;
#endif

    delete mpVol_RT_Tex;
    mpVol_RT_Tex = nullptr;

    delete mpVol_RT_Rbo;
    mpVol_RT_Rbo = nullptr;

    delete mpVol_RT_Fbo;
    mpVol_RT_Fbo = nullptr;

    delete mpLight_Ubo;
    mpLight_Ubo = nullptr;

    gfxUtils::freeMeshGfxBuffers( mScreenQuadHandle );
    gfxUtils::freeMeshGfxBuffers( mScreenTriHandle );

    mSharedMem.put( "DVR_WatchdogTime_shouldRun", "false" );
    mpWatchdogThread->join();
    delete mpWatchdogThread;
    mpWatchdogThread = nullptr;

    if (mpTFProcess) { mSharedMem.put( "stopTransferFunctionApp", "true" ); int exitStatus = mpTFProcess->get_exit_status(); }
    delete mpTFProcess;
    mpTFProcess = nullptr;

    delete mpSCProcess;
    mpSCProcess = nullptr;
}

Status_t ApplicationDVR::load( const std::string& fileUrl, const int32_t gradientMode ) {
    mDataFileUrl = fileUrl;

    delete mpData;
    mpData = new VolumeData;
    mpData->load( fileUrl.c_str(), static_cast< FileLoader::VolumeData::gradientMode_t >( gradientMode ) );

    constexpr int32_t mipLvl = 0;

    const auto volDim = mpData->getDim();
    const auto texDim = linAlg::i32vec3_t{ volDim[0], volDim[1], volDim[2] };
    GfxAPI::Texture::Desc_t volTexDesc{
        .texDim = texDim,
        .numChannels = 1,
        .channelType = GfxAPI::eChannelType::u16,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    delete mpDensityTex3d;
    mpDensityTex3d = new GfxAPI::Texture;
    mpDensityTex3d->create( volTexDesc );
    mpDensityTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 0 );
    mpDensityTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 1 );
    mpDensityTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 2 );
    mpDensityTex3d->setBorderColor( { 0.0f, 0.0f, 0.0f, 0.0f } );
    mpDensityTex3d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );

    GfxAPI::Texture::Desc_t gradientTexDesc{
        .texDim = texDim,
        //.numChannels = 2,
        .numChannels = 3,
        //.channelType = GfxAPI::eChannelType::f32,
        .channelType = GfxAPI::eChannelType::f16,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    delete mpGradientTex3d;
    mpGradientTex3d = new GfxAPI::Texture;
    mpGradientTex3d->create( gradientTexDesc );
    mpGradientTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 0 );
    mpGradientTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 1 );
    mpGradientTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 2 );
    mpGradientTex3d->uploadData( mpData->getNormals().data(), GL_RGB, GL_FLOAT, mipLvl );

    mpData->calculateHistogramBuckets();
    const auto& histoBuckets = mpData->getHistoBuckets();

    mSharedMem.put( "volTexDim3D", reinterpret_cast<const uint8_t *const>( texDim.data() ), static_cast<uint32_t>( texDim.size() * sizeof( texDim[0] ) ) );
    mSharedMem.put( "histoBuckets", reinterpret_cast<const uint8_t *const>( histoBuckets.data() ), static_cast<uint32_t>( histoBuckets.size() * sizeof( histoBuckets[0] ) ) );
    mSharedMem.put( "histoBucketsDirty", "true" );

    return Status_t::OK();
}

void ApplicationDVR::setRotationPivotPos(   linAlg::vec3_t& rotPivotPosOS, 
                                                linAlg::vec3_t& rotPivotPosWS, 
                                                const int32_t& fbWidth, const int32_t& fbHeight, 
                                                const float currMouseX, const float currMouseY ) {
    glFlush();
    glFinish();

    glCheckError();
    printf( "\n" );

    glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
    if (glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE) {
        printf( "framebuffer not complete!\n" );
    }
    glReadBuffer( GL_BACK );

    const GLint glReadX = static_cast<GLint>(currMouseX); 
    const GLint glReadY = (fbHeight - 1) - static_cast<GLint>(currMouseY); // flip y for reading from framebuffer
    
    const float fReadWindowRelativeX = ( currMouseX / static_cast<float>( fbWidth - 1 ) );
    const float fReadWindowRelativeY = ( currMouseY / static_cast<float>( fbHeight - 1 ) );


#if 0 // read color
    std::array< uint8_t, 4 > colorAtMouseCursor;
    glReadPixels( glReadX, glReadY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, colorAtMouseCursor.data() );
    printf( "mouse read color = %u %u %u %u at (%f, %f) for window size (%d, %d)\n", 
        colorAtMouseCursor[0], colorAtMouseCursor[1], colorAtMouseCursor[2], colorAtMouseCursor[3], 
        currMouseX, currMouseY, fbWidth, fbHeight );
#endif

    float depthAtMousePos = 0.0f;
    glReadPixels( glReadX, glReadY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthAtMousePos );

    printf( "mouse read depth = %f at (%f, %f) for window size (%d, %d)\n", depthAtMousePos, currMouseX, currMouseY, fbWidth, fbHeight );

    linAlg::vec4_t mousePosNDC{ fReadWindowRelativeX * 2.0f - 1.0f, ( 1.0f - fReadWindowRelativeY ) * 2.0f - 1.0f, depthAtMousePos * 2.0f - 1.0f, 1.0f };
    //printVec( "mousePosNDC", mousePosNDC );

    linAlg::vec3_t prevPivotWS = rotPivotPosWS;

    linAlg::mat4_t invMvpMatrix;
    linAlg::inverse( invMvpMatrix, mMvpMatrix );

    mousePosNDC = invMvpMatrix * mousePosNDC;

    auto mousePosOS = mousePosNDC;
    mousePosOS[3] = linAlg::maximum( mousePosOS[3], std::numeric_limits<float>::epsilon() );
    mousePosOS[0] /= mousePosOS[3];
    mousePosOS[1] /= mousePosOS[3];
    mousePosOS[2] /= mousePosOS[3];
    //printVec( "mousePosOS", mousePosOS );

    linAlg::vec4_t modelCenterAndRadius{0.0f,0.0f,0.0f,5.0f};
    // if (!mStlModels.empty()) {
    //     mStlModels[0].getBoundingSphere( modelCenterAndRadius );
    // }
    //printf( "OS bounding sphere center (%f, %f, %f), radius %f\n", modelCenterAndRadius[0], modelCenterAndRadius[1], modelCenterAndRadius[2], modelCenterAndRadius[3] );


    printVec( "prev rotPivotPosOS", rotPivotPosOS );
    linAlg::vec3_t distToPrevPivotPosOS;
    linAlg::sub( distToPrevPivotPosOS, linAlg::vec3_t{ mousePosOS[0], mousePosOS[1], mousePosOS[2] }, rotPivotPosOS );
    printVec( "distToPrevPivotPosOS", distToPrevPivotPosOS );

    rotPivotPosOS[0] = mousePosOS[0];
    rotPivotPosOS[1] = mousePosOS[1];
    rotPivotPosOS[2] = mousePosOS[2];
    //printVec( "rotationPivotPos OS", rotationPivotPos );
    printVec( "new rotPivotPosOS", rotPivotPosOS );

#if 1 // transform to WS
    rotPivotPosWS = rotPivotPosOS;
    linAlg::applyTransformationToVector( mModelMatrix3x4, &rotPivotPosWS, 1 );
    printVec( "rot pivot pos WS", rotPivotPosWS );
#endif

    //linAlg::mat3x4_t blah = linAlg::mat3x4( mMvpMatrix );

    //linAlg::vec3_t pivotDiff;
    //linAlg::sub( pivotDiff, prevPivotWS, rotPivotPosWS );

    //panVector[0] += pivotDiff[0];
    //panVector[1] += pivotDiff[1];
    //panVector[2] += pivotDiff[2];

    ////linAlg::vec3_t prevES = prevPivotWS;
    ////linAlg::applyTransformationToPoint( mViewMatrix3x4, &prevES, 1 );
    //linAlg::vec4_t sphereCenterRadius;
    //mStlModels[0].getBoundingSphere( sphereCenterRadius );
    //linAlg::vec3_t prevES{ sphereCenterRadius[0], sphereCenterRadius[1], sphereCenterRadius[2] }; // still in OS (or WS?)
    //linAlg::applyTransformationToPoint( prevViewMatrix3x4, &prevES, 1 );

    ////linAlg::vec3_t currES = rotPivotPosWS;
    ////linAlg::applyTransformationToPoint( mViewMatrix3x4, &currES, 1 );

    //linAlg::vec3_t currES{ sphereCenterRadius[0], sphereCenterRadius[1], sphereCenterRadius[2] }; // still in OS (or WS?)
    //linAlg::applyTransformationToPoint( mViewMatrix3x4, &currES, 1 );

    ////pivotCompensationES
    //panVector = panVector + ( currES - prevES );

    glCheckError();
    printf( "\n" );
}

void ApplicationDVR::LoadDVR_Shaders(   const DVR_GUI::eVisAlgo visAlgo, 
                                        const DVR_GUI::eDebugVisMode debugVisMode, 
                                        const bool useEmptySpaceSkipping, 
                                        GfxAPI::Shader& meshShader, 
                                        GfxAPI::Shader& volShader )
{
    meshShader.~Shader();
    volShader.~Shader();

    std::string defineStr;
    
    if (visAlgo == DVR_GUI::eVisAlgo::levoyIsosurface) {
        defineStr = "-DDVR_MODE=LEVOY_ISO_SURFACE";
    } else if (visAlgo == DVR_GUI::eVisAlgo::f2bCompositing) {
        defineStr = "-DDVR_MODE=F2B_COMPOSITE";
    } else if (visAlgo == DVR_GUI::eVisAlgo::xray) {
        defineStr = "-DDVR_MODE=XRAY";
    } else if (visAlgo == DVR_GUI::eVisAlgo::mri) {
        defineStr = "-DDVR_MODE=MRI";
    }
    
    defineStr.append( " " );
    if (debugVisMode == DVR_GUI::eDebugVisMode::none) {
        defineStr.append( "-DDEBUG_VIS_MODE=DEBUG_VIS_NONE" );
    } else if (debugVisMode == DVR_GUI::eDebugVisMode::bricks) {
        defineStr.append( "-DDEBUG_VIS_MODE=DEBUG_VIS_BRICKS" );
    } else if (debugVisMode == DVR_GUI::eDebugVisMode::relativeCost) {
        defineStr.append( "-DDEBUG_VIS_MODE=DEBUG_VIS_RELCOST" );
    } else if (debugVisMode == DVR_GUI::eDebugVisMode::stepsSkipped) {
        defineStr.append( "-DDEBUG_VIS_MODE=DEBUG_VIS_STEPSSKIPPED" );
    } else if (debugVisMode == DVR_GUI::eDebugVisMode::invStepsSkipped) {
        defineStr.append( "-DDEBUG_VIS_MODE=DEBUG_VIS_INVSTEPSSKIPPED" );
    }

    defineStr.append( " " );
    if (useEmptySpaceSkipping) {
        defineStr.append( "-DUSE_EMPTY_SPACE_SKIPPING=1" );
    } else {
        defineStr.append( "-DUSE_EMPTY_SPACE_SKIPPING=0" );
    }

    mSharedMem.put( "SHADER_DEFINES", defineStr );
    tryStartShaderCompilerApp();


    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > meshShaderDesc{
        std::make_pair( "./src/shaders/rayMarchUnitCube.vert.glsl.preprocessed", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/rayMarchUnitCube.frag.glsl.preprocessed", GfxAPI::Shader::eShaderStage::FS ),
    };
    gfxUtils::createShader( meshShader, meshShaderDesc );
    meshShader.use( true );
    meshShader.setInt( "u_densityTex", 0 );
    meshShader.setInt( "u_gradientTex", 1 );
    meshShader.setInt( "u_densityLoResTex", 2 );
    meshShader.setInt( "u_emptySpaceTex", 3 );
    meshShader.setInt( "u_colorAndAlphaTex", 7 );
    meshShader.setFloat( "u_recipTexDim", 1.0f );
    meshShader.setVec2( "u_surfaceIsoAndThickness", surfaceIsoAndThickness );
    meshShader.use( false );

    printf( "creating volume shader\n" ); fflush( stdout );
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > volShaderDesc{
        std::make_pair( "./src/shaders/tex3d-raycast.vert.glsl.preprocessed", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/tex3d-raycast.frag.glsl.preprocessed", GfxAPI::Shader::eShaderStage::FS ), 
    };
    gfxUtils::createShader( volShader, volShaderDesc );
    volShader.use( true );
    volShader.setInt( "u_densityTex", 0 );
    volShader.setInt( "u_gradientTex", 1 );
    volShader.setInt( "u_densityLoResTex", 2 );
    volShader.setInt( "u_emptySpaceTex", 3 );
    volShader.setInt( "u_colorAndAlphaTex", 7 );
    volShader.setFloat( "u_recipTexDim", 1.0f );
    volShader.setVec2( "u_surfaceIsoAndThickness", surfaceIsoAndThickness );
    volShader.use( false );

    const uint32_t uboLightParamsShaderBindingIdx = 0; // ogl >= 420 ==> layout(std140, binding = 2), and here uboLightParamsShaderBindingIdx would be 2

    const auto meshShaderIdx = static_cast<uint32_t>(meshShader.programHandle());
    uint32_t meshShaderLightParamsUboIdx = glGetUniformBlockIndex( meshShaderIdx, "LightParameters" );
    glUniformBlockBinding( meshShaderIdx, meshShaderLightParamsUboIdx, uboLightParamsShaderBindingIdx );

    const auto volShaderIdx = static_cast<uint32_t>(volShader.programHandle());
    uint32_t volShaderLightParamsUboIdx = glGetUniformBlockIndex( volShaderIdx, "LightParameters" );
    glUniformBlockBinding( meshShaderIdx, volShaderLightParamsUboIdx, uboLightParamsShaderBindingIdx );

    //glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboExampleBlock);
    //glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboExampleBlock, 0, 152);
    glBindBufferRange( GL_UNIFORM_BUFFER, uboLightParamsShaderBindingIdx, static_cast<uint32_t>(mpLight_Ubo->handle()), 0, mpLight_Ubo->desc().numBytes );

    if (mpData != nullptr) {
        fixupShaders( meshShader, volShader );
    }
}

void ApplicationDVR::fixupShaders( GfxAPI::Shader& meshShader, GfxAPI::Shader& volShader ) {
    glCheckError();
    const auto& minMaxDensity = mpData->getMinMaxDensity();

    const auto minMaxScaleVal = linAlg::vec3_t{ 
        static_cast<float>(minMaxDensity[0]) / 65535.0f, 
        static_cast<float>(minMaxDensity[1]) / 65535.0f, 
        1.0f / (static_cast<float>(minMaxDensity[1] - minMaxDensity[0]) / 65535.0f) };

    const auto volDim = mpData->getDim();
    const auto maxVolDim = linAlg::maximum( volDim[0], linAlg::maximum( volDim[1], volDim[2] ) );
    const auto recipTexDim = 1.0f / linAlg::maximum( 1.0f, static_cast<float>(maxVolDim) );

    volShader.use( true );
    volShader.setVec3( "u_minMaxScaleVal", minMaxScaleVal );
    volShader.setFloat( "u_recipTexDim", recipTexDim );
    volShader.use( false );

    meshShader.use( true );
    meshShader.setVec3( "u_minMaxScaleVal", minMaxScaleVal );
    meshShader.setFloat( "u_recipTexDim", recipTexDim );
    meshShader.use( false );

    printf( "min max densities %u %u\n", (uint32_t)minMaxDensity[0], (uint32_t)minMaxDensity[1] );
    glCheckError();


    //mpDensityTex3d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );
    const auto volDataDim = mpData->getDim();

    mVolLoResEmptySpaceSkipDim = { 
        ( volDataDim[0] + ( volBrickDim[0] - 1 ) ) / volBrickDim[0],
        ( volDataDim[1] + ( volBrickDim[1] - 1 ) ) / volBrickDim[1],
        ( volDataDim[2] + ( volBrickDim[2] - 1 ) ) / volBrickDim[2],
    };

    mVolLoResData.clear();
    mVolLoResData.resize( mVolLoResEmptySpaceSkipDim[0] * mVolLoResEmptySpaceSkipDim[1] * mVolLoResEmptySpaceSkipDim[2] );

#pragma omp parallel for schedule(dynamic, 1)		// OpenMP 
    for (int32_t z = 0; z < volDataDim[2]; z += volBrickDim[2]) { // error C3016: 'z': index variable in OpenMP 'for' statement must have signed integral type
        for (int32_t y = 0; y < volDataDim[1]; y += volBrickDim[1]) {
            for (int32_t x = 0; x < volDataDim[0]; x += volBrickDim[0]) {

                // TODO: find max transparency
                //      - for now, just use the density, later map density to transparency through TF
                uint16_t maxVal = 0;
                uint16_t minVal = std::numeric_limits<uint16_t>::max();
                uint32_t numVoxelsVisited = 0;
                for (int32_t bz = 0; bz < volBrickDim[2]; bz++) { // error C3016: 'z': index variable in OpenMP 'for' statement must have signed integral type
                    const auto hiResZ = z + bz;
                    if (hiResZ >= volDataDim[2]) { break; }
                    for (int32_t by = 0; by < volBrickDim[1]; by++) {
                        const auto hiResY = y + by;
                        if (hiResY >= volDataDim[1]) { break; }
                        for (int32_t bx = 0; bx < volBrickDim[0]; bx++) {
                            const auto hiResX = x + bx;
                            if (hiResX >= volDataDim[0]) { break; }
                            
                            //const uint32_t addrHiRes = (hiResZ * volDataDim[1] + hiResY) * volDataDim[0] + hiResX;
                            const uint32_t addrHiRes = mpData->calcAddr( hiResX, hiResY, hiResZ );

                            uint16_t currVal = mpData->getDensities()[addrHiRes]; // for now, just use the density, later map density to transparency through TF
                            maxVal = linAlg::maximum( maxVal, currVal );
                            minVal = linAlg::minimum( minVal, currVal );
                            numVoxelsVisited++;
                        }
                    }
                }

                const uint32_t loResX = x / volBrickDim[0];
                const uint32_t loResY = y / volBrickDim[1];
                const uint32_t loResZ = z / volBrickDim[2];

                //mVolLoResData[(loResZ * mVolLoResEmptySpaceSkipDim[1] + loResY) * mVolLoResEmptySpaceSkipDim[0] + loResX] = maxVal;
                mVolLoResData[(loResZ * mVolLoResEmptySpaceSkipDim[1] + loResY) * mVolLoResEmptySpaceSkipDim[0] + loResX] = { minVal, maxVal };
            }
        }
    }
    
    GfxAPI::Texture::Desc_t volLoResTexDesc{
        .texDim = mVolLoResEmptySpaceSkipDim,
        .numChannels = 2,
        .channelType = GfxAPI::eChannelType::u16,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    const uint32_t mipLvl = 0;

    delete mpDensityLoResTex3d;
    mpDensityLoResTex3d = new GfxAPI::Texture;
    mpDensityLoResTex3d->create( volLoResTexDesc );
    mpDensityLoResTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 0 );
    mpDensityLoResTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 1 );
    mpDensityLoResTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 2 );
    const auto minFilter = GfxAPI::eFilterMode::box;
    const auto magFilter = GfxAPI::eFilterMode::box;
    const auto mipFilter = GfxAPI::eFilterMode::box;
    mpDensityLoResTex3d->setFilterMode( minFilter, magFilter, mipFilter );
    mpDensityLoResTex3d->uploadData( mVolLoResData.data(), GL_RG, GL_UNSIGNED_SHORT, mipLvl );


    visibleBricksWithDists.clear();
    visibleBricksWithDists.reserve( mVolLoResEmptySpaceSkipDim[0] * mVolLoResEmptySpaceSkipDim[1] * mVolLoResEmptySpaceSkipDim[2] );

    threadBsd.clear();
    threadBsd.resize( omp_get_max_threads() );
    for (auto& entry : threadBsd) {
        entry.reserve( ( mVolLoResEmptySpaceSkipDim[0] * mVolLoResEmptySpaceSkipDim[1] * mVolLoResEmptySpaceSkipDim[2] ) / omp_get_num_threads() );
    }

}

Status_t ApplicationDVR::run() {
    ArcBallControls arcBallControl;
    const ArcBall::ArcBallControls::InteractionModeDesc arcBallControlInteractionSettings{ .fullCircle = false, .smooth = false };
    arcBallControl.setDampingFactor( 0.845f );
    
    // handle window resize
    int32_t fbWidth, fbHeight;
    glfwGetFramebufferSize( reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ), &fbWidth, &fbHeight);
    //printf( "glfwGetFramebufferSize: %d x %d\n", fbWidth, fbHeight );

    mScreenQuadHandle = gfxUtils::createScreenQuadGfxBuffers();

    // load shaders

    StlModel stlModel;
    stlModel.load( "./src/data/blender-cube-ascii.STL" );
    //stlModel.load( "./data/blender-cube.STL" );

    GfxAPI::Shader meshShader;
    GfxAPI::Shader volShader;

    mStlModelHandle = gfxUtils::createMeshGfxBuffers(
        stlModel.coords().size() / 3,
        stlModel.coords(),
        stlModel.normals().size() / 3,
        stlModel.normals(),
        stlModel.indices().size(),
        stlModel.indices() );
    
    LoadDVR_Shaders( visAlgo, DVR_GUI::eDebugVisMode::none, true, meshShader, volShader );



    printf( "creating triangle-fullscreen shader\n" ); fflush( stdout );
    GfxAPI::Shader fullscreenTriShader;
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > fullscreenTriShaderDesc{
        std::make_pair( "./src/shaders/fullscreenTri.vert.glsl.preprocessed", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/fullscreenTri.frag.glsl.preprocessed", GfxAPI::Shader::eShaderStage::FS ), // X-ray of x-y planes
    };
    gfxUtils::createShader( fullscreenTriShader, fullscreenTriShaderDesc );
    fullscreenTriShader.use( true );
    fullscreenTriShader.setInt(   "u_Tex", 0 );
    fullscreenTriShader.use( false );


    GLFWwindow *const pWindow = reinterpret_cast< GLFWwindow *const >( mContextOpenGL.window() );

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); // not necessary
    glDisable( GL_CULL_FACE );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    
    int32_t prevFbWidth = -1;
    int32_t prevFbHeight = -1;
    
    constexpr int32_t numSrcChannels = 3;
    std::shared_ptr< uint8_t > pGrabbedFramebuffer = std::shared_ptr< uint8_t >( new uint8_t[ fbWidth * fbHeight * numSrcChannels ], std::default_delete< uint8_t[] >() );

    glEnable( GL_DEPTH_TEST );

    glfwSwapInterval( 1 );    // should sync to display framerate

    //glfwSetKeyCallback( pWindow, keyboardCallback ); // // don't set this, since otherwise ImGui text fields will no longer recognize BACKSPACE, DEL, ARROW-KEYS and manybe a couple others..
    glfwSetScrollCallback( pWindow, mouseWheelCallback );
    
    glfwSetFramebufferSizeCallback( pWindow, framebufferSizeCallback );

    glCheckError();

    linAlg::vec3_t axis_mx{ 0.0f, 1.0f, 0.0f }; // initial model rotation
    linAlg::vec3_t axis_my{ 1.0f, 0.0f, 0.0f }; // initial model rotation

    float camTiltRadAngle = 0.0f;
    float targetCamTiltRadAngle = camTiltRadAngle;

    float prevMouseX, prevMouseY;
    float currMouseX, currMouseY;
    double dCurrMouseX, dCurrMouseY;
    glfwGetCursorPos( pWindow, &dCurrMouseX, &dCurrMouseY );
    currMouseX = static_cast<float>(dCurrMouseX);
    currMouseY = static_cast<float>(dCurrMouseY);
    prevMouseX = currMouseX;
    prevMouseY = currMouseY;

    float targetMouse_dx = 0.0f;
    float targetMouse_dy = 0.0f;

    bool leftMouseButton_down = false;

    linAlg::mat3x4_t tiltRotMat;
    linAlg::loadIdentityMatrix( tiltRotMat );



    tryStartTransferFunctionApp(); // start TF process

    { // colors
        const GfxAPI::Texture::Desc_t texDesc = ApplicationDVR_common::densityColorsTexDesc();
        delete mpDensityColorsTex2d;
        mpDensityColorsTex2d = new GfxAPI::Texture;
        mpDensityColorsTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
        mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 0 );
        mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToEdge, 1 );
        //mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToBorder, 1 );
    }


    bool guiWantsMouseCapture = false;
    //std::array<uint8_t, 1024 * 4> interpolatedDensityColorsCPU;
    std::vector<uint8_t> interpolatedDensityColorsCPU;
    interpolatedDensityColorsCPU.resize( 1024 * 4 );

    int gradientCalculationAlgoIdx = (mpData == nullptr) ? static_cast<int>(FileLoader::VolumeData::gradientMode_t::SOBEL_3D ) : static_cast<int>(mpData->getGradientMode());
    
    std::vector<bool> prevCollapsedState;
    bool wasCollapsedToggled = false;

    bool lightParamsChanged = true; // force first update
    DVR_GUI::LightParameters lightParams{
        .lightDir = linAlg::normalizeRet( linAlg::vec4_t{ 0.2f, 0.7f, -0.1f, 0.0f } ),
        .lightColor = linAlg::vec4_t{0.95f, 0.8f, 0.8f, 0.0f},
        .ambient = 0.01f,
        .diffuse = 0.8f,
        .specular = 0.95f,
    };

    std::array<float, 8> frameDurations;
    std::fill( frameDurations.begin(), frameDurations.end(), 1.0f / 60.0f );

    bool useEmptySpaceSkipping = true;
    auto debugVisMode = DVR_GUI::eDebugVisMode::none;

    bool prevDidMove = false;
    uint64_t frameNum = 0;
    while( !glfwWindowShouldClose( pWindow ) ) {
        
        bool didMove = false;

        const auto frameStartTime = std::chrono::system_clock::now();
        std::time_t newt = std::chrono::system_clock::to_time_t(frameStartTime);
        mSharedMem.put( "DVR-app-time", std::to_string( newt ) );

        if ( /*frameNum % 3 == 0 &&*/ mSharedMem.get( "TFdirty" ) == "true" ) {
            didMove = true;
            //std::array<uint8_t, 1024 * 4> interpolatedDensityColorsCPU;
            uint32_t bytesRead;
            bool retVal = mSharedMem.get( "TFcolorsAndAlpha", interpolatedDensityColorsCPU.data(), static_cast<uint32_t>( interpolatedDensityColorsCPU.size() ), &bytesRead );
            if ( retVal ) {
                assert( retVal == true && bytesRead == interpolatedDensityColorsCPU.size() );

                mpDensityColorsTex2d->uploadData( interpolatedDensityColorsCPU.data(), GL_RGBA, GL_UNSIGNED_BYTE, 0 );

                // TODO: create empty space skipping table => entry in table x..minVal, y..maxVal => search transparency between the minVal and maxVal density and if any val with transparency > 1 exists, break and set table entry to false (no space skipping, since not empty space)
                {
                    mEmptySpaceTableData.clear();
                    mEmptySpaceTableData.resize( 1024 * 1024 );
                    std::fill( mEmptySpaceTableData.begin(), mEmptySpaceTableData.end(), 255 );
                    for ( uint32_t y = 0; y < 1024; y++ ) { // start at first significant hounsfield unit
                        
                        for (uint32_t x = 0; x < 1024; x++) {
                            const uint32_t addr = y * 1024 + x;
                            if (x > y) { // makes no sense when x..minVal bigger than y..maxVal
                                continue;
                            }
                            bool foundPosTransparency = false;
                            for (uint32_t densityIdx = x; densityIdx <= y; densityIdx++) {
                                const auto alpha = interpolatedDensityColorsCPU[densityIdx * 4 + 3];
                                //if (alpha > 0) {
                                if (alpha > 0) {
                                    foundPosTransparency = true;
                                    break;
                                }
                            }
                            if (foundPosTransparency) {
                                mEmptySpaceTableData[addr] = 0;
                            }
                        }
                    }
                    const uint32_t mipLvl = 0;
                    mpEmptySpaceTex2d->uploadData( mEmptySpaceTableData.data(), GL_RED, GL_UNSIGNED_BYTE, mipLvl );
                    
                }

                mSharedMem.put( "TFdirty", "false" );
            }
        }

        zoomSpeed = zoomSpeedBase;

        glfwPollEvents();
        processInput( pWindow );
        glfwGetCursorPos( pWindow, &dCurrMouseX, &dCurrMouseY );
        currMouseX = static_cast<float>(dCurrMouseX);
        currMouseY = static_cast<float>(dCurrMouseY);
        const float mouse_dx = (currMouseX - prevMouseX) * mouseSensitivity;
        const float mouse_dy = (currMouseY - prevMouseY) * mouseSensitivity;

        bool leftMouseButtonPressed   = ( glfwGetMouseButton( pWindow, GLFW_MOUSE_BUTTON_1 ) == GLFW_PRESS );
        bool middleMouseButtonPressed = ( glfwGetMouseButton( pWindow, GLFW_MOUSE_BUTTON_3 ) == GLFW_PRESS );
        bool rightMouseButtonPressed  = ( glfwGetMouseButton( pWindow, GLFW_MOUSE_BUTTON_2 ) == GLFW_PRESS );


        
        if (fabsf( camZoomDist - targetCamZoomDist ) > FLT_EPSILON * 1000.0f ) { didMove = true; }
        if (fabsf( targetCamTiltRadAngle - camTiltRadAngle ) > FLT_EPSILON * 1000.0f ) { didMove = true; }
        if (fabsf( targetPanDeltaVector[0] ) > FLT_EPSILON * 1000.0f ) { didMove = true; }
        if (fabsf( targetPanDeltaVector[1] ) > FLT_EPSILON * 1000.0f ) { didMove = true; }
        if ( ( leftMouseButtonPressed || middleMouseButtonPressed || rightMouseButtonPressed ) ) { didMove = true; }


        static uint8_t setNewRotationPivot = 0;

        if ( ( leftMouseButtonPressed && !guiWantsMouseCapture ) || rightMouseButtonPressed || middleMouseButtonPressed ) {
            if (mGrabCursor) { glfwSetInputMode( pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED ); }
            if (leftMouseButton_down == false && pressedOrRepeat( pWindow, GLFW_KEY_LEFT_CONTROL )) { // LMB was just freshly pressed
                setNewRotationPivot = 2;
            }
            leftMouseButton_down = true;
        } else {
            glfwSetInputMode( pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
            leftMouseButton_down = false;
        }

        arcBallControl.setActive( !guiWantsMouseCapture );

        if (rightMouseButtonPressed) {
            //printf( "RMB pressed!\n" );
            targetCamZoomDist -= mouse_dy / ( fbHeight * mouseSensitivity * 0.5f );
            targetCamTiltRadAngle -= mouse_dx / (fbWidth * mouseSensitivity * 0.5f);
        }
        if (middleMouseButtonPressed) {
            //printf( "MMB pressed!\n" );
            targetPanDeltaVector[0] += mouse_dx * frameDelta * mouseSensitivity;
            targetPanDeltaVector[1] -= mouse_dy * frameDelta * mouseSensitivity;
        }

        glCheckError();

        linAlg::mat3_t rolledRefFrameMatT;
        {
            linAlg::mat3x4_t camRollMat;
            linAlg::loadRotationZMatrix( camRollMat, camTiltRadAngle );

            linAlg::mat3_t rolledRefFrameMat;
            linAlg::vec3_t refX;
            linAlg::castVector( refX, camRollMat[0] );
            linAlg::vec3_t refY;
            linAlg::castVector( refY, camRollMat[1] );
            linAlg::vec3_t refZ;
            linAlg::castVector( refZ, camRollMat[2] );
            rolledRefFrameMat[0] = refX;
            rolledRefFrameMat[1] = refY;
            rolledRefFrameMat[2] = refZ;

            linAlg::transpose( rolledRefFrameMatT, rolledRefFrameMat );

            //linAlg::loadIdentityMatrix( rolledRefFrameMatT ); //!!!! REMOVE AGAIN
            arcBallControl.setRefFrameMat( rolledRefFrameMatT );
            //!!! arcBallControl.setCamTiltRadAngle( testTiltRadAngle );
        }

        //arcBallControl.setRotationPivotOffset( rotPivotPosWS );
        arcBallControl.update( frameDelta, currMouseX, currMouseY, leftMouseButtonPressed, rightMouseButtonPressed, fbWidth, fbHeight );
        arcBallControl.setRotationPivotOffset( rotPivotPosWS );

        const float arc_dead = arcBallControl.getDeadZone();
        const float arc_dx = arcBallControl.getTargetMovement_dx();
        const float arc_dy = arcBallControl.getTargetMovement_dy();
        const bool arc_isUpdating = ( arc_dx >= 10.0f * arc_dead || arc_dy >= 10.0f * arc_dead );
        didMove = didMove || arc_isUpdating;

        const linAlg::mat3x4_t& arcRotMat = arcBallControl.getRotationMatrix();

        linAlg::vec4_t boundingSphere;
        linAlg::vec3_t volDimRatio{ 1.0f, 1.0f, 1.0f };

        //???
        {

            constexpr float modelScaleFactor = 1.0f;
            linAlg::mat3x4_t scaleMatrix;
            linAlg::loadScaleMatrix( scaleMatrix, linAlg::vec3_t{ modelScaleFactor, modelScaleFactor, modelScaleFactor } );

            stlModel.getBoundingSphere( boundingSphere );
            // linAlg::loadTranslationMatrix( centerTranslationMatrix, linAlg::vec3_t{ -boundingSphere[0], -boundingSphere[1], -boundingSphere[2] } );
            
            if (mpData != nullptr) {
                const auto dataDim = mpData->getDim();
                const float dimX = static_cast<float>(dataDim[0]);
                const float dimY = static_cast<float>(dataDim[1]);
                const float dimZ = static_cast<float>(dataDim[2]);
                const float maxDim = linAlg::maximum( dimX, linAlg::maximum( dimY, dimZ ) );

                volDimRatio = linAlg::vec3_t{ dimX / maxDim, dimY / maxDim, dimZ / maxDim };
                
                if (rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster) {
                    // DEBUG!!!
                    //volDimRatio = volDimRatio * 0.5f;
                    linAlg::loadScaleMatrix( scaleMatrix, volDimRatio ); // unit cube approach
                    //linAlg::loadIdentityMatrix( scaleMatrix );
                }
            }
        
            
            // MODEL matrix => place center of object's bounding sphere at world origin
            linAlg::mat3x4_t boundingSphereCenter_Translation_ModelMatrix;
            linAlg::loadTranslationMatrix(
                boundingSphereCenter_Translation_ModelMatrix,
                linAlg::vec3_t{ -boundingSphere[0], -boundingSphere[1], -boundingSphere[2] } );

            linAlg::mat3x4_t rotX180deg_ModelMatrix;
            linAlg::loadRotationXMatrix( rotX180deg_ModelMatrix, static_cast<float>( M_PI ) );

            mModelMatrix3x4 = arcRotMat * rotX180deg_ModelMatrix * boundingSphereCenter_Translation_ModelMatrix * scaleMatrix;

            // ### VIEW MATRIX ###

            linAlg::vec3_t zAxis{ 0.0f, 0.0f, 1.0f };
            linAlg::loadRotationZMatrix( tiltRotMat, camTiltRadAngle );

        #if 1 // works (up to small jumps when resetting pivot anker pos); uses transform from last frame (seems to be okay as well)
            linAlg::mat3x4_t pivotTranslationMatrix;
            auto pivotTranslationPos = linAlg::vec3_t{ -rotPivotPosWS[0], -rotPivotPosWS[1], -rotPivotPosWS[2] };
            linAlg::loadTranslationMatrix( pivotTranslationMatrix, pivotTranslationPos );
            linAlg::mat3x4_t invPivotTranslationMatrix;
            auto invPivotTranslationPos = linAlg::vec3_t{ -pivotTranslationPos[0], -pivotTranslationPos[1], -pivotTranslationPos[2] };
            linAlg::loadTranslationMatrix( invPivotTranslationMatrix, invPivotTranslationPos );
            tiltRotMat = invPivotTranslationMatrix * tiltRotMat * pivotTranslationMatrix;
        #endif

            mViewMatrix3x4 = tiltRotMat;

            if (arcBallControlInteractionSettings.smooth) {
                panVector[0] += targetPanDeltaVector[0];
                panVector[1] += targetPanDeltaVector[1];
            } else {
                panVector[0] += targetPanDeltaVector[0] / ( panDamping );
                panVector[1] += targetPanDeltaVector[1] / ( panDamping );
                targetPanDeltaVector[0] = 0.0f;
                targetPanDeltaVector[1] = 0.0f;
            }
            const float viewDist = camZoomDist;
            linAlg::vec3_t panVec3{ panVector[0], panVector[1], -boundingSphere[3] * viewDist + panVector[2] };
            //linAlg::vec3_t panVec3{ panVector[0] - tiltRotMat[0][3], panVector[1] - tiltRotMat[1][3], -boundingSphere[3] * viewDist - tiltRotMat[2][3] };
            
            linAlg::mat3x4_t panMat;
            linAlg::loadTranslationMatrix( panMat, panVec3 );
           
            mViewMatrix3x4 = panMat * mViewMatrix3x4;

            linAlg::mat3x4_t mvMat3x4 = mViewMatrix3x4 * mModelMatrix3x4;

            linAlg::castMatrix(mModelViewMatrix, mvMat3x4);

        //??? 
        }

        // update mvp matrix
        linAlg::multMatrix( mMvpMatrix, mProjMatrix, mModelViewMatrix );

        handleScreenResize(
            reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ),
            mProjMatrix, 
            prevFbWidth,
            prevFbHeight,
            fbWidth,
            fbHeight );

        glfwGetFramebufferSize( reinterpret_cast<GLFWwindow*>(mContextOpenGL.window()), &fbWidth, &fbHeight );

        if (prevFbWidth != fbWidth || prevFbHeight != fbHeight) 
        {
            calculateFovYProjectionMatrix( fbWidth, fbHeight, fovY_deg, mProjMatrix );

            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
        }

        glCheckError();

        didMove = didMove || wasCollapsedToggled;

        if (didMove || prevDidMove) {
            glDisable( GL_BLEND );
        }
        else {
            glEnable( GL_BLEND );
            glBlendEquation( GL_FUNC_ADD );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }

        glEnable( GL_DEPTH_TEST );
        //glDepthFunc( GL_LESS );
        glDepthFunc( GL_LEQUAL );
        
        mpVol_RT_Fbo->bind( true );
        //glBindFramebuffer( GL_FRAMEBUFFER, 0 );

        glViewport( 0, 0, fbWidth, fbHeight ); // set to render-target size
        { // clear screen
            
            //constexpr float clearColorValue[]{ 0.0f, 0.0f, 0.0f, 1.0f }; // setting alpha to 1 is essential for OpenGL-based F2B compositing

            const float dstAlpha = (rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster) ? 1.0f : 0.5f; // setting alpha to 1 is essential for OpenGL-based F2B compositing
            const float clearColorValue[]{ 0.0f, 0.0f, 0.0f, dstAlpha }; 
            
            //!!! constexpr float clearColorValue[]{ 0.0f, 0.0f, 0.0f, 0.5f };
            //constexpr float clearColorValue[]{ 0.0f, 0.0f, 0.0f, 0.0f };

            if (didMove || prevDidMove ) {
                glClearBufferfv( GL_COLOR, 0, clearColorValue );
            }

            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
            //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        }

        if (setNewRotationPivot == 1) { // fixup cam-jump compensation movement when new pivot is selected and cam tilt angle is not zero
            linAlg::vec3_t currRefPtES{ 0.0f, 0.0f, 0.0f };
            linAlg::applyTransformationToPoint( mViewMatrix3x4, &currRefPtES, 1 );
            auto compensationVec = currRefPtES - prevRefPtES;
            panVector = panVector - compensationVec; // persist changes to future frames

            // apply only compensation difference vector in this frame on top of the previously calculated transform
            linAlg::mat3x4_t compensationMat3x4;
            linAlg::loadTranslationMatrix( compensationMat3x4, -1.0f * compensationVec );
            mViewMatrix3x4 = compensationMat3x4 * mViewMatrix3x4; // fixed up view matrix

            // update matrices that depend on view matrix to have a consistently fixed-up transform
            linAlg::mat3x4_t mvMat3x4 = mViewMatrix3x4 * mModelMatrix3x4;
            linAlg::castMatrix( mModelViewMatrix, mvMat3x4 );
            linAlg::multMatrix( mMvpMatrix, mProjMatrix, mModelViewMatrix );
        }

        // TODO: draw screen-aligned quad to lauch one ray per pixel
        //       dual view on transformation: inverse model-view transform should place cam frame such that it sees the model as usual, but now model is in origin and axis aligned
        //       this should make data traversal easier...


        mpDensityColorsTex2d->bindToTexUnit( 7 );

        if (!mDataFileUrl.empty()) {

            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->bindToTexUnit( 0 );
                mpGradientTex3d->bindToTexUnit( 1 );
            }

            if (mpDensityLoResTex3d != nullptr) {
                mpDensityLoResTex3d->bindToTexUnit( 2 );
            }
            if (mpEmptySpaceTex2d != nullptr) {
                mpEmptySpaceTex2d->bindToTexUnit( 3 );
            }

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, mModelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformationToPoint( invModelViewMatrix, &camPos_OS, 1 );

            // use inverse transpose ???
            //linAlg::mat4_t invTransposeModelViewMatrix;
            //linAlg::transpose( invTransposeModelViewMatrix, invModelViewMatrix );

            const auto vecTransformMat = invModelViewMatrix;
            //const auto vecTransformMat = invTransposeModelViewMatrix;

            constexpr linAlg::vec4_t cam_ES_z = { 0.0f, 0.0f, 1.0f, 1.0f };
            linAlg::vec4_t cam_OS_p_z = cam_ES_z;
            linAlg::applyTransformationToPoint( vecTransformMat, &cam_OS_p_z, 1 );
            linAlg::vec4_t cam_OS_z;
            linAlg::sub( cam_OS_z, cam_OS_p_z, camPos_OS );

            // OS-plane parallel to view plane through camera pos
            linAlg::vec3_t viewPlane_normal_OS{ -cam_OS_z[0], -cam_OS_z[1], -cam_OS_z[2] };
            linAlg::normalize( viewPlane_normal_OS );
            linAlg::vec4_t viewPlane_OS{ viewPlane_normal_OS[0], 
                                         viewPlane_normal_OS[1], 
                                         viewPlane_normal_OS[2], 
                                         -linAlg::dot( viewPlane_normal_OS, {camPos_OS[0], camPos_OS[1], camPos_OS[2] } ) };


        #if 1 // unit-cube STL file
            if (rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster) {
                glEnable( GL_CULL_FACE );
                glCullFace( GL_FRONT ); // <<< !!!
                //glCullFace( GL_BACK );

                glBindVertexArray( mStlModelHandle.vaoHandle );
                meshShader.use( true );

                auto retSetUniform = meshShader.setMat4( "u_mvpMat", mMvpMatrix );

                //meshShader.setInt( "u_frameNum", frameNum );
                if (!didMove) {
                    meshShader.setInt( "u_frameNum", rand() % 10000 );
                }
                meshShader.setVec2( "u_surfaceIsoAndThickness", surfaceIsoAndThickness );

                meshShader.setVec4( "u_camPos_OS", camPos_OS );
                //meshShader.setVec3( "u_volDimRatio", volDimRatio ); //!!!

                if (!useEmptySpaceSkipping) {
                    meshShader.setVec3( "u_volDimRatio", { 1.0f, 1.0f, 1.0f } ); //!!!
                    meshShader.setVec3( "u_volOffset", { 0.0f, 0.0f, 0.0f } ); //!!!
                    glDrawElements( GL_TRIANGLES, static_cast<GLsizei>(stlModel.indices().size()), GL_UNSIGNED_INT, 0 );
                }
                else {
                    glEnable( GL_BLEND );
                    //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); // B2F

                    // https://community.khronos.org/t/front-to-back-blending/65155
                    // https://developer.download.nvidia.com/SDK/10/opengl/src/dual_depth_peeling/doc/DualDepthPeeling.pdf
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE,GL_ZERO,GL_ONE_MINUS_SRC_ALPHA); // F2B
                    //glClearColor( 0.0, 0.0, 0.0, 1.0 );
                    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


                    //glBlendFunc( GL_ONE, GL_ONE);
                    {
                        //float scaleFactor = 1.0f / brickLen;
                        //int32_t scaleBrickLen = 2 * brickLen;
                        //float scaleFactor = 1.0f / scaleBrickLen;
                        //meshShader.setVec3( "u_volDimRatio", linAlg::vec3_t{ scaleFactor, scaleFactor, scaleFactor } );
                        //meshShader.setVec3( "u_volDimRatio", linAlg::vec3_t{ 
                        //    1.0f / ( mVolLoResEmptySpaceSkipDim[0] /*- 1*/ ), 
                        //    1.0f / ( mVolLoResEmptySpaceSkipDim[1] /*- 1*/ ),
                        //    1.0f / ( mVolLoResEmptySpaceSkipDim[2] /*- 1*/ )} );


                        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

                    #if 0
                        for (int32_t z = 0; z < mVolLoResEmptySpaceSkipDim[2]; z++) {
                            for (int32_t y = 0; y < mVolLoResEmptySpaceSkipDim[1]; y++) {
                                for (int32_t x = 0; x < mVolLoResEmptySpaceSkipDim[0]; x++) {
                                    meshShader.setVec3( "u_volOffset", {
                                        (float)(x) / (mVolLoResEmptySpaceSkipDim[0] /*- 1*/),
                                        (float)(y) / (mVolLoResEmptySpaceSkipDim[1] /*- 1*/),
                                        (float)(z) / (mVolLoResEmptySpaceSkipDim[2] /*- 1*/) } );
                                    glDrawElements( GL_TRIANGLES, static_cast<GLsizei>(stlModel.indices().size()), GL_UNSIGNED_INT, 0 );
                                }
                            }
                    }
                    #else
                        const auto hiResDimOrig = mpData->getDim();

                        auto hiResDim = mpData->getDim();
                        hiResDim[0] = ((hiResDim[0] + (brickLen - 1)) / brickLen) * brickLen;
                        hiResDim[1] = ((hiResDim[1] + (brickLen - 1)) / brickLen) * brickLen;
                        hiResDim[2] = ((hiResDim[2] + (brickLen - 1)) / brickLen) * brickLen;

                        meshShader.setVec3( "u_volDimRatio", linAlg::vec3_t{
                            (float)(brickLen) / (hiResDimOrig[0] /*- 1*/),
                            (float)(brickLen) / (hiResDimOrig[1] /*- 1*/),
                            (float)(brickLen) / (hiResDimOrig[2] /*- 1*/) } );


                        visibleBricksWithDists.clear();
                        //visibleBricksWithDists.reserve( mVolLoResEmptySpaceSkipDim[0] * mVolLoResEmptySpaceSkipDim[1] * mVolLoResEmptySpaceSkipDim[2] );

                        const linAlg::vec3_t camPos3_OS{ camPos_OS[0], camPos_OS[1], camPos_OS[2] };
                        
                        //visibleBricksWithDists.resize(mVolLoResEmptySpaceSkipDim[0] * mVolLoResEmptySpaceSkipDim[1] * mVolLoResEmptySpaceSkipDim[2]);
                        //int k = 0;
                        for (auto& entry : threadBsd) {
                            entry.clear();
                        }

                        //const float lenCamOS_z = linAlg::len( linAlg::vec3_t{ cam_OS_z[0], cam_OS_z[1], cam_OS_z[2] } );
                        //int32_t maxDim = -1;
                        //linAlg::vec3_t absCam_OS_z{ abs(cam_OS_z[0]), abs(cam_OS_z[1]), abs(cam_OS_z[2]) };
                        //if (absCam_OS_z[0] > absCam_OS_z[1]) {
                        //    if (absCam_OS_z[0] > absCam_OS_z[2]) {
                        //        maxDim = 0; // X
                        //    } else {
                        //        maxDim = 2; // Z
                        //    }
                        //}
                        //else {
                        //    if (absCam_OS_z[1] > absCam_OS_z[2]) {
                        //        maxDim = 1; // Y
                        //    } else {
                        //        maxDim = 2; // Z
                        //    }
                        //}

                        const linAlg::vec3_t refPt{
                            ( camPos3_OS[0] * 0.5f + 0.5f ) * hiResDimOrig[0] - 0.5f / brickLen,
                            ( camPos3_OS[1] * 0.5f + 0.5f ) * hiResDimOrig[1] - 0.5f / brickLen,
                            ( camPos3_OS[2] * 0.5f + 0.5f ) * hiResDimOrig[2] - 0.5f / brickLen };

                        // for ref - how it's calculated in the [-1;+1] OS range
                        //linAlg::vec4_t viewPlane_OS{ 
                        //    viewPlane_normal_OS[0], 
                        //    viewPlane_normal_OS[1], 
                        //    viewPlane_normal_OS[2], 
                        //    -linAlg::dot( viewPlane_normal_OS, {camPos_OS[0], camPos_OS[1], camPos_OS[2] } ) };

                        const linAlg::vec3_t viewPlaneNormalRefOS{ // undo - this time just the scaling (multiply and divide)
                            ( viewPlane_OS[0] * 0.5f ) * hiResDimOrig[0] /* - 0.5f / brickLen */,
                            ( viewPlane_OS[1] * 0.5f ) * hiResDimOrig[1] /* - 0.5f / brickLen */,
                            ( viewPlane_OS[2] * 0.5f ) * hiResDimOrig[2] /* - 0.5f / brickLen */ };

                        const linAlg::vec4_t viewPlaneRef_OS{
                            viewPlaneNormalRefOS[0],
                            viewPlaneNormalRefOS[1],
                            viewPlaneNormalRefOS[2],
                            -linAlg::dot( viewPlaneNormalRefOS, refPt ) };

                        //int threadId;
                    #pragma omp parallel for schedule(dynamic, 1) //private(threadId)		// OpenMP 
                    //#pragma omp parallel for shared(k, visibleBricksWithDists)
                        for (int32_t z = 0; z < hiResDim[2]; z += brickLen) {
                            for (int32_t y = 0; y < hiResDim[1]; y += brickLen) {
                                for (int32_t x = 0; x < hiResDim[0]; x += brickLen) {

                                    {// BRICK SKIP CPU
                                        const auto lx = x / brickLen;
                                        const auto ly = y / brickLen;
                                        const auto lz = z / brickLen;
                                        const auto minMaxDensity = mVolLoResData[(lz * mVolLoResEmptySpaceSkipDim[1] + ly) * mVolLoResEmptySpaceSkipDim[0] + lx];
                                        //if (mEmptySpaceTableData[((minMaxDensity[1] + 2) / 4) * 1024 + (minMaxDensity[0] + 2) / 4] > 127) { continue; }
                                        if (mEmptySpaceTableData[((minMaxDensity[1] + 0) / 4) * 1024 + (minMaxDensity[0] + 0) / 4] > 127) { continue; }
                                    }

                                    const float fx = static_cast<float>( x );
                                    const float fy = static_cast<float>( y );
                                    const float fz = static_cast<float>( z );

                                    float minPositiveDist = std::numeric_limits<float>::max();
                                    bool isPositive = false;
                                    {
                                    #if 1
                                        const linAlg::vec4_t brickCenterOS = { fx, fy, fz, 1.0f };

                                        const float distBrickCornerToViewPlane_OS = linAlg::dot( viewPlaneRef_OS, brickCenterOS );
                                    #else // play it safe, but should be slower...
                                        const linAlg::vec4_t brickCenterOS = { ( fx + 0.5f * brickLen ) / (hiResDimOrig[0] ) * 2.0f - 1.0f ,
                                                                               ( fy + 0.5f * brickLen ) / (hiResDimOrig[1] ) * 2.0f - 1.0f ,
                                                                               ( fz + 0.5f * brickLen ) / (hiResDimOrig[2] ) * 2.0f - 1.0f ,
                                                                               1.0f };

                                        const float distBrickCornerToViewPlane_OS = linAlg::dot( viewPlane_OS, brickCenterOS );
                                    #endif
                                        if (distBrickCornerToViewPlane_OS >= 0.0f) {
                                            minPositiveDist = distBrickCornerToViewPlane_OS;
                                            isPositive = true;
                                        } 
                                    }
                                    if (!isPositive) { continue; } // none of the brick's corners is in front of camera - skip this brick

                                    {
                                    //#pragma omp critical // synched push_back
                                        std::lock_guard<std::mutex> lk(visibleBricksWithDists_mutex);
                                        //visibleBricksWithDists.push_back( { // works ootb with concurrent_vector
                                        //    .x = x,
                                        //    .y = y,
                                        //    .z = z,
                                        //    .distToNearPlane = minPositiveDist,
                                        //    //.distFromViewRay = 0.0f /*minDistToViewRay*/ } );
                                        //    .distFromViewRay = minDistToViewRay } );

                                        //visibleBricksWithDists[k] = { // works ootb with concurrent_vector
                                        //    .x = x,
                                        //    .y = y,
                                        //    .z = z,
                                        //    .distToNearPlane = minPositiveDist,
                                        //    .distFromViewRay = minDistToViewRay };
                                        //k++;

                                        int ompThreadNum = omp_get_thread_num();
                                        threadBsd[ompThreadNum].push_back( { // works ootb with concurrent_vector
                                                .x = x,
                                                .y = y,
                                                .z = z } );
                                    }
                                }
                            }
                        }

                        //visibleBricksWithDists.resize( k );
                        for (const auto& entry : threadBsd) {
                            for (const auto& subEntry : entry) {
                                visibleBricksWithDists.push_back( subEntry );
                            }
                        }

                    #if 1
                        //const linAlg::vec3_t refPt{
                        //    ( camPos3_OS[0] * 0.5f + 0.5f ) * hiResDimOrig[0] - 0.5f / brickLen,
                        //    ( camPos3_OS[1] * 0.5f + 0.5f ) * hiResDimOrig[1] - 0.5f / brickLen,
                        //    ( camPos3_OS[2] * 0.5f + 0.5f ) * hiResDimOrig[2] - 0.5f / brickLen };

                        //std::sort(  std::execution::par_unseq, visibleBricksWithDists.begin(),
                        std::sort(  std::execution::par, 
                                    std::begin(visibleBricksWithDists),
                                    std::end(visibleBricksWithDists),
                                    [&]( const brickSortData_t& a, const brickSortData_t& b ) {
                                        linAlg::vec3_t currPtA{ static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z) };
                                        linAlg::vec3_t currPtB{ static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z) };
                                        return linAlg::dist( refPt, currPtA ) < linAlg::dist( refPt, currPtB );
                                    } );
                    #else
                        std::sort(  visibleBricksWithDists.begin(),
                            visibleBricksWithDists.end(),
                            [=]( const brickSortData_t& a, const brickSortData_t& b ) {
                                linAlg::vec3_t currPtA{ 
                                    ( a.x + 0.5f * brickLen ) / ( hiResDimOrig[0] ) * 2.0f - 1.0f ,
                                    ( a.y + 0.5f * brickLen ) / ( hiResDimOrig[1] ) * 2.0f - 1.0f ,
                                    ( a.z + 0.5f * brickLen ) / ( hiResDimOrig[2] ) * 2.0f - 1.0f };
                                linAlg::vec3_t currPtB{ 
                                    ( b.x + 0.5f * brickLen ) / ( hiResDimOrig[0] ) * 2.0f - 1.0f ,
                                    ( b.y + 0.5f * brickLen ) / ( hiResDimOrig[1] ) * 2.0f - 1.0f ,
                                    ( b.z + 0.5f * brickLen ) / ( hiResDimOrig[2] ) * 2.0f - 1.0f };

                                return linAlg::dist( camPos3_OS, currPtA ) < linAlg::dist( camPos3_OS, currPtB );
                            } );
                    #endif

                        //glDisable( GL_DEPTH_TEST );
                        glEnable( GL_DEPTH_TEST );
                    
                        for (auto& entry : visibleBricksWithDists) { // these should now render with correct brick-vis even without depth buffer
                            meshShader.setVec3( "u_volOffset", { // !!! <<<
                                (float)entry.x / (hiResDimOrig[0]),
                                (float)entry.y / (hiResDimOrig[1]),
                                (float)entry.z / (hiResDimOrig[2]) } );

                            glDrawElements( GL_TRIANGLES, static_cast<GLsizei>(stlModel.indices().size()), GL_UNSIGNED_INT, 0 );
                        }
                        glEnable( GL_DEPTH_TEST );

                    #endif
                        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
                    }

                    if (didMove || prevDidMove) {
                        glDisable( GL_BLEND );
                    }
                    else {
                        glEnable( GL_BLEND );
                        glBlendEquation( GL_FUNC_ADD );
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    }

                }


                meshShader.use( false );
                glBindVertexArray( 0 );
                glDisable( GL_CULL_FACE );
            }
        #endif

        #if 1 
            if (rayMarchAlgo == DVR_GUI::eRayMarchAlgo::fullscreenBoxIsect) {
                glDisable( GL_CULL_FACE );
                volShader.use( true );
                glBindVertexArray( mScreenQuadHandle.vaoHandle );

                //volShader.setInt( "u_frameNum", frameNum );
                if (!didMove) {
                    volShader.setInt( "u_frameNum", rand() % 10000 );
                }
                volShader.setVec4( "u_camPos_OS", camPos_OS );
                volShader.setVec3( "u_volDimRatio", volDimRatio );
                volShader.setVec2( "u_surfaceIsoAndThickness", surfaceIsoAndThickness );

                linAlg::mat4_t invModelViewProjectionMatrix;
                linAlg::inverse( invModelViewProjectionMatrix, mMvpMatrix );

                constexpr float fpDist_NDC = +1.0f;
                std::array< linAlg::vec4_t, 4 > cornersFarPlane_NDC{
                    linAlg::vec4_t{ -1.0f, +1.0f, fpDist_NDC, 1.0f }, // L top
                    linAlg::vec4_t{ -1.0f, -1.0f, fpDist_NDC, 1.0f }, // L bottom
                    linAlg::vec4_t{ +1.0f, -1.0f, fpDist_NDC, 1.0f }, // R bottom
                    linAlg::vec4_t{ +1.0f, +1.0f, fpDist_NDC, 1.0f }, // R top
                };

                // transform points to Object Space of Volume Data
                linAlg::applyTransformationToPoint( invModelViewProjectionMatrix, cornersFarPlane_NDC.data(), cornersFarPlane_NDC.size() );

                volShader.setVec4Array( "u_fpDist_OS", cornersFarPlane_NDC.data(), 4 );

                glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );

                volShader.use( false );
                glBindVertexArray( 0 );
            }
        #endif

            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->unbindFromTexUnit();
                mpGradientTex3d->unbindFromTexUnit();
            }
            if (mpDensityLoResTex3d != nullptr) {
                mpDensityLoResTex3d->unbindFromTexUnit();
            }
            if (mpEmptySpaceTex2d != nullptr) {
                mpEmptySpaceTex2d->unbindFromTexUnit();
            }
        }


        mpVol_RT_Fbo->bind( false );

        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        glViewport( 0, 0, fbWidth, fbHeight ); // set to render-target size
        { // clear screen
            constexpr float clearColorValue[]{ 0.0f, 0.0f, 0.0f, 0.5f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
        }

        glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );
        glDisable( GL_BLEND );
        fullscreenTriShader.use( true );
        mpVol_RT_Tex->bindToTexUnit( 0 );
        glBindVertexArray( mScreenTriHandle.vaoHandle );
        glDrawArrays( GL_TRIANGLES, 0, 3 );
        mpVol_RT_Tex->unbindFromTexUnit();
        fullscreenTriShader.use( false );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );



        glCheckError();

        if (setNewRotationPivot > 0) {
            if (setNewRotationPivot > 1) {
                prevRefPtES = { 0.0f, 0.0f, 0.0f };
                linAlg::applyTransformationToPoint( mViewMatrix3x4, &prevRefPtES, 1 );
                setRotationPivotPos( rotPivotPosOS, rotPivotPosWS, fbWidth, fbHeight, currMouseX, currMouseY );
            } 
            setNewRotationPivot--;
        }


        {
            static bool loadFileTrigger = false;
            static bool resetTrafos = false;
            
            int visAlgoIdx = static_cast<int>(visAlgo);
            int* pVisAlgoIdx = &visAlgoIdx;

            int debugVisModeIdx = static_cast<int>(debugVisMode);
            int* pDebugVisModeIdx = &debugVisModeIdx;

            int rayMarchAlgoIdx = static_cast<int>(rayMarchAlgo);
            int* pRayMarchAlgoIdx = &rayMarchAlgoIdx;

            bool editTransferFunction = false;
            
            std::array<int, 3> dimArray = (mpData == nullptr) ? std::array<int, 3>{0, 0, 0} : std::array<int, 3>{ mpData->getDim()[0], mpData->getDim()[1], mpData->getDim()[2] };

            const uint32_t colorIdx = linAlg::clamp<uint32_t>( 
                static_cast<uint32_t>( surfaceIsoAndThickness[0] * ( interpolatedDensityColorsCPU.size()/4u ) ), 
                0u, 
                static_cast<uint32_t>( interpolatedDensityColorsCPU.size() )/4u - 1u );

            std::array<float,3> isoColor{
                1.0f / 255.0f * interpolatedDensityColorsCPU[ colorIdx * 4 + 0 ],
                1.0f / 255.0f * interpolatedDensityColorsCPU[ colorIdx * 4 + 1 ],
                1.0f / 255.0f * interpolatedDensityColorsCPU[ colorIdx * 4 + 2 ],
            };

            bool prevUseEmptySpaceSkipping = useEmptySpaceSkipping;
            DVR_GUI::GuiUserData_t guiUserData{
                .volumeDataUrl = mDataFileUrl,
                .pGradientModeIdx = &gradientCalculationAlgoIdx,
                .pSharedMem = &mSharedMem,
                .frameRate = 1.0f / frameDurations[frameDurations.size()/2u],
                .pVisAlgoIdx = pVisAlgoIdx,
                .pDebugVisModeIdx = pDebugVisModeIdx,
                .useEmptySpaceSkipping = useEmptySpaceSkipping,
                .pRayMarchAlgoIdx = pRayMarchAlgoIdx,
                .loadFileTrigger = loadFileTrigger,
                .resetTrafos = resetTrafos,
                .wantsToCaptureMouse = guiWantsMouseCapture,
                .editTransferFunction = editTransferFunction,
                .dim = dimArray,
                .surfaceIsoAndThickness = surfaceIsoAndThickness,
                .surfaceIsoColor = isoColor,
                .lightParams = lightParams,
                .lightParamsChanged = lightParamsChanged,
            };
            

            std::vector<bool> collapsedState;
            
        #if 1
            DVR_GUI::DisplayGui( &guiUserData, collapsedState, fbWidth, fbHeight );
        #else
            mpGuiFbo->bind( true );
            {
                constexpr float clearColorValue[]{ 0.0f, 0.0f, 0.0f, 0.0f };
                glClearBufferfv( GL_COLOR, 0, clearColorValue );
            }

            DVR_GUI::DisplayGui( &guiUserData, collapsedState );

            mpGuiFbo->bind( false );

            glEnable( GL_BLEND );
            glBlendEquation( GL_MAX );
            glBlendFunc( GL_SRC_ALPHA, GL_SRC_ALPHA );
            glDisable( GL_DEPTH_TEST );
            glDepthMask( GL_FALSE );
            fullscreenTriShader.use( true );
            mpGuiTex->bindToTexUnit( 0 );
            glBindVertexArray( mScreenTriHandle.vaoHandle );
            glDrawArrays( GL_TRIANGLES, 0, 3 );
            mpGuiTex->unbindFromTexUnit();
            fullscreenTriShader.use( false );
            glEnable( GL_DEPTH_TEST );
            glDepthMask( GL_TRUE );
        #endif

            if (guiUserData.lightParamsChanged) {
                void *pLightUbo = mpLight_Ubo->map( GfxAPI::eMapMode::writeOnly );
                //memcpy( pLightUbo, &lightParams, sizeof( lightParams ) );
                memcpy( pLightUbo, &lightParams, mpLight_Ubo->desc().numBytes );
                mpLight_Ubo->unmap();
                lightParamsChanged = false;
            }


            wasCollapsedToggled = false;
            if (frameNum > 0) {
                const size_t prevSize = prevCollapsedState.size();
                const size_t currSize = collapsedState.size();
                if (prevSize == currSize) {
                    for (size_t i = 0; i < currSize; i++) {
                        if (prevCollapsedState[i] != collapsedState[i]) {
                            wasCollapsedToggled = true;
                            break;
                        }
                    }
                } else {
                    wasCollapsedToggled = true;
                }
            }
            prevCollapsedState = collapsedState;

            if (*pVisAlgoIdx != static_cast<int>(visAlgo)) {
                visAlgo = (DVR_GUI::eVisAlgo)(*pVisAlgoIdx);
                printf( "visAlgoIdx = %d, visAlgo = %d\n", *pVisAlgoIdx, (int)visAlgo );

                // spawn shader compiler and re-load DVR shaders

                //#define DVR_MODE                LEVOY_ISO_SURFACE
                ////#define DVR_MODE                F2B_COMPOSITE
                ////#define DVR_MODE                XRAY
                ////#define DVR_MODE                MRI

                LoadDVR_Shaders( visAlgo, debugVisMode, useEmptySpaceSkipping, meshShader, volShader );
                didMove = true;
            }

            if (*pDebugVisModeIdx != static_cast<int>(debugVisMode)) {
                debugVisMode = (DVR_GUI::eDebugVisMode)(*pDebugVisModeIdx);
                printf( "debugVisModeIdx = %d, debugVisMode = %d\n", *pDebugVisModeIdx, (int)debugVisMode );

                // spawn shader compiler and re-load DVR shaders
                LoadDVR_Shaders( visAlgo, debugVisMode, useEmptySpaceSkipping, meshShader, volShader );
                didMove = true;
            }

            if (prevUseEmptySpaceSkipping != useEmptySpaceSkipping) {
                LoadDVR_Shaders( visAlgo, debugVisMode, useEmptySpaceSkipping, meshShader, volShader );
                didMove = true;
            }

            if (*pRayMarchAlgoIdx != static_cast<int>(rayMarchAlgo)) {
                rayMarchAlgo = (DVR_GUI::eRayMarchAlgo)(*pRayMarchAlgoIdx);
                printf( "rayMarchAlgoIdx = %d, rayMarchAlgo = %d\n", *pRayMarchAlgoIdx, (int)rayMarchAlgo );
                didMove = true;
            }

            if (loadFileTrigger) {
                load( mDataFileUrl, gradientCalculationAlgoIdx );
                
                fixupShaders( meshShader, volShader );

                resetTransformations( arcBallControl, camTiltRadAngle, targetCamTiltRadAngle );

                loadFileTrigger = false;
                didMove = true;
            }

            if ( mpData != nullptr && gradientCalculationAlgoIdx >= 0 &&  gradientCalculationAlgoIdx != static_cast<int>(mpData->getGradientMode()) ) {
                const uint32_t mipLvl = 0;
                mpData->calculateNormals( static_cast<FileLoader::VolumeData::gradientMode_t>( gradientCalculationAlgoIdx ) );
                mpGradientTex3d->uploadData( mpData->getNormals().data(), GL_RGB, GL_FLOAT, mipLvl );
                didMove = true;
            }

            if (resetTrafos) {
                printf( "reset Trafos!\n" );
                resetTransformations( arcBallControl, camTiltRadAngle, targetCamTiltRadAngle );
                resetTrafos = false;
                didMove = true;
            }
            
            mSharedMem.put( "surfaceIsoAndThickness", 
                            reinterpret_cast<const uint8_t *const>( surfaceIsoAndThickness.data() ), 
                            static_cast<uint32_t>( surfaceIsoAndThickness.size() * sizeof( surfaceIsoAndThickness[0] ) ) );

            if (guiUserData.editTransferFunction) {
                tryStartTransferFunctionApp();
            }

        }

        //glfwWaitEventsTimeout( 0.032f ); // wait time is in seconds
        glfwSwapBuffers( pWindow );

        const auto frameEndTime = std::chrono::system_clock::now();
        frameDelta = static_cast<float>(std::chrono::duration_cast<std::chrono::duration<double>>(frameEndTime - frameStartTime).count());

        frameDurations[frameNum % frameDurations.size()] = frameDelta;
        std::sort( frameDurations.begin(), frameDurations.end() );

        frameDelta = linAlg::minimum( frameDelta, 0.032f );

        targetCamZoomDist += mouseWheelOffset * zoomSpeed * boundingSphere[3]*0.05f;
        //targetCamZoomDist = linAlg::clamp( targetCamZoomDist, 0.1f, 1000.0f );
        camZoomDist = targetCamZoomDist * (1.0f - angleDamping) + camZoomDist * angleDamping;
        mouseWheelOffset = 0.0f;

        camTiltRadAngle = targetCamTiltRadAngle * (1.0f - angleDamping) + camTiltRadAngle * angleDamping;

        linAlg::scale( targetPanDeltaVector, (1.0f - panDamping) );

        prevMouseX = currMouseX;
        prevMouseY = currMouseY;

        prevDidMove = didMove;

        //if ( frameNum % 200 == 0 ) {
        //    const auto queriedSmVal = mSharedMem.get( "from TF" );

        //    printf( "DVR app - what we got for \"from TF\": %s\n", queriedSmVal.c_str() );
        //}

        frameNum++;
    }

    DVR_GUI::DestroyGui();

    #if 0
        std::stringstream commentStream;
        commentStream << "# grabbed framebuffer: " << 1 << "\n";
        constexpr bool ppmBinary = false;
        netBpmIO::writePpm( "grabbedFramebuffer.ppm", pGrabbedFramebuffer.get(), numSrcChannels, fbWidth, fbHeight, ppmBinary, commentStream.str() );
    #endif

    return Status_t::OK();
}

void ApplicationDVR::tryStartTransferFunctionApp() {

    if ( mpData != nullptr ) {
        const auto& texDim =  mpDensityTex3d->desc().texDim;
        const auto& histoBuckets = mpData->getHistoBuckets();

        mSharedMem.put( "volTexDim3D", reinterpret_cast<const uint8_t *const>( texDim.data() ), static_cast<uint32_t>( texDim.size() * sizeof( texDim[0] ) ) );
        mSharedMem.put( "histoBuckets", reinterpret_cast<const uint8_t *const>( histoBuckets.data() ), static_cast<uint32_t>( histoBuckets.size() * sizeof( histoBuckets[0] ) ) );
        mSharedMem.put( "histoBucketsDirty", "true" );
    }

    int exitStatus;
    if (mpTFProcess == nullptr || mpTFProcess->try_get_exit_status( exitStatus )) {
        if (mpTFProcess) { mpTFProcess->kill(); }
        delete mpTFProcess;
        mpTFProcess = new TinyProcessLib::Process( mTFCmdLinePath );
    }
}

void ApplicationDVR::tryStartShaderCompilerApp() {

    int exitStatus;
    if (mpSCProcess == nullptr || mpSCProcess->try_get_exit_status( exitStatus )) {
        if (mpSCProcess) { mpSCProcess->kill(); }
        delete mpSCProcess;
        mpSCProcess = new TinyProcessLib::Process( mSCCmdLinePath );
    }
    mpSCProcess->get_exit_status();
}

void ApplicationDVR::resetTransformations( ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle ) {
    arcBallControl.resetTrafos();
    camTiltRadAngle = 0.0f;
    targetCamTiltRadAngle = 0.0f;
    panVector = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    targetPanDeltaVector = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    rotPivotPosOS = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    rotPivotPosWS = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    rotPivotOffset = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    camZoomDist = 0.0f;
    targetCamZoomDist = initialCamZoomDist;
}

void ApplicationDVR::handleScreenResize( 
    GLFWwindow *const pWindow,
    linAlg::mat4_t& projMatrix, 
    int32_t& prevFbWidth,
    int32_t& prevFbHeight,
    int32_t& fbWidth,
    int32_t& fbHeight ) {

        {
            int32_t windowW, windowH;
            glfwGetWindowSize( reinterpret_cast<GLFWwindow*>(pWindow), &windowW, &windowH );
            //printf( "glfwGetWindowSize(): window created: %d x %d\n", windowW, windowH ); fflush( stdout );

            float sx, sy;
            glfwGetWindowContentScale( reinterpret_cast<GLFWwindow*>(pWindow), &sx, &sy );
            //printf( "glfwGetWindowContentScale(): window scale: %f x %f\n", sx, sy ); fflush( stdout );

            //printf( "scaled window dimensions %f x %f\n", windowW * sx, windowH * sy );

            int32_t fbWidth, fbHeight;
            glfwGetFramebufferSize( reinterpret_cast<GLFWwindow*>(pWindow), &fbWidth, &fbHeight );
            //printf( "glfwGetFramebufferSize(): %d x %d\n", fbWidth, fbHeight );

            glViewport( 0, 0, windowW, windowH );
        }

        glfwGetFramebufferSize( pWindow, &fbWidth, &fbHeight);

        if ( prevFbWidth != fbWidth || prevFbHeight != fbHeight ) {

            printf( "polled new window size %d x %d\n", fbWidth, fbHeight );

            //calculateProjectionMatrix( fbWidth, fbHeight, projMatrix );
            calculateFovYProjectionMatrix( fbWidth, fbHeight, fovY_deg, projMatrix );

            GfxAPI::Texture::Desc_t guiTexDesc{
                .texDim = {fbWidth, fbHeight, 0},
                .numChannels = 4,
                .channelType = GfxAPI::eChannelType::u8,
                .semantics = GfxAPI::eSemantics::color,
                .isMipMapped = false,
            };
        #if 0
            delete mpGuiTex;
            mpGuiTex = nullptr;
            mpGuiTex = new GfxAPI::Texture( guiTexDesc );

            GfxAPI::Fbo::Desc guiFboDesc;
            guiFboDesc.colorAttachments.push_back( mpGuiTex );

            delete mpGuiFbo;
            mpGuiFbo = nullptr;
            mpGuiFbo = new GfxAPI::Fbo( guiFboDesc );
        #endif


            GfxAPI::Texture::Desc_t volRT_TexDesc{
                .texDim = {fbWidth, fbHeight, 0},
                .numChannels = 4,
                .channelType = GfxAPI::eChannelType::u8,
                .semantics = GfxAPI::eSemantics::color,
                .isMipMapped = false,
            };
            delete mpVol_RT_Tex;
            mpVol_RT_Tex = nullptr;
            mpVol_RT_Tex = new GfxAPI::Texture( volRT_TexDesc );

            GfxAPI::Rbo::Desc volRT_RboDesc;
            volRT_RboDesc.w = fbWidth;
            volRT_RboDesc.h = fbHeight;
            volRT_RboDesc.numChannels = 1;
            volRT_RboDesc.channelType = GfxAPI::eChannelType::f32depth;
            volRT_RboDesc.semantics = GfxAPI::eSemantics::depth;

            delete mpVol_RT_Rbo;
            mpVol_RT_Rbo = nullptr;
            mpVol_RT_Rbo = new GfxAPI::Rbo( volRT_RboDesc );

            GfxAPI::Fbo::Desc volRT_FboDesc;
            volRT_FboDesc.colorAttachments.push_back( mpVol_RT_Tex );
            volRT_FboDesc.depthStencilAttachment = nullptr;

            delete mpVol_RT_Fbo;
            mpVol_RT_Fbo = nullptr;
            mpVol_RT_Fbo = new GfxAPI::Fbo( volRT_FboDesc );
            mpVol_RT_Rbo->attachToFbo( *mpVol_RT_Fbo, 0 );

            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
        }
}
