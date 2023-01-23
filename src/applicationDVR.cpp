// TODO: 
// for the unit-cube raymarcher: handle cam-in-box case => Problem is glaub ich nur die near plane, die dann die backfaces beginnt wegzuklippen weshalb ich dann die ray-exit Schnittpunkte verlier (und somit den gesamten Strahl durch dieses Pixel)
// noisy offset um banding artifacts zu minimieren - m�gl.weise gepaart mit dem Ansatz zum Sobel-normal filtering

#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/checkErrorGL.h"

#include <math.h>
#include <iostream>
#include <tchar.h>
#include <filesystem>

//#include <process.h> // for ::GetCommandLine
//#include <namedpipeapi.h>
//#include <Windows.h>


#include "ApplicationDVR.h"
#include "fileLoaders/volumeData.h"
#include "fileLoaders/offLoader.h"
#include "fileLoaders/stlModel.h" // used for the unit-cube


#include "arcBall/arcBallControls.h"

#include "GUI/DVR_GUI.h"

//#include "external/tiny-process-library/process.hpp"


#include <memory>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>


#include <cassert>

#include "glad/glad.h"
#include "GLFW/glfw3.h"


using namespace ArcBall;

namespace {
    static DVR_GUI::eRayMarchAlgo rayMarchAlgo = DVR_GUI::eRayMarchAlgo::fullscreenBoxIsect;

    constexpr static float fovY_deg = 60.0f;

    constexpr float angleDamping = 0.85f;
    constexpr float panDamping = 0.25f;
    constexpr float mouseSensitivity = 0.23f;
    constexpr float zoomSpeed = 1.5f;

    static linAlg::vec3_t pivotCompensationES{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t rotPivotOffset{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t rotPivotPosOS{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t rotPivotPosWS{ 0.0f, 0.0f, 0.0f };

    //static linAlg::vec4_t prevSphereCenterES{ 0.0f, 0.0f, 0.0f, 1.0f };
    static linAlg::vec3_t prevRefPtES{ 0.0f, 0.0f, 0.0f };

    linAlg::mat3x4_t prevModelMatrix3x4;
    linAlg::mat3x4_t prevViewMatrix3x4;

    constexpr static float initialCamZoomDist = 4.5f;
    constexpr static float initialCamZoomDistTestSuite = 2.0f;
    static float camZoomDist = 0.0f;
    static float targetCamZoomDist = initialCamZoomDist;

    static linAlg::vec3_t targetPanDeltaVector{ 0.0f, 0.0f, 0.0f };
    static linAlg::vec3_t panVector{ 0.0f, 0.0f, 0.0f };

    static constexpr float camSpeedFact = 1.0f; // should be a certain fraction of the bounding sphere radius
    static float frameDelta = 0.016f; // TODO: actually calculate frame duration in the main loop

    static constexpr linAlg::vec4_t greenScreenColor{ 0.0f, 1.0f, 1.0f, 1.0f };

    static std::string readFile( const std::string &filePath ) { 
        std::ifstream ifile{ filePath.c_str() };
        return std::string( std::istreambuf_iterator< char >( ifile ), 
                            std::istreambuf_iterator< char >() );
    }

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
        if ( pressedOrRepeat( pWindow, GLFW_KEY_LEFT_SHIFT ) )  { camSpeed *= 5.0f; }
        if ( pressedOrRepeat( pWindow, GLFW_KEY_RIGHT_SHIFT ) ) { camSpeed *= 0.1f; }

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

    static void keyboardCallback( GLFWwindow *const pWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods ) {
        //if ( key == GLFW_KEY_E && action == GLFW_RELEASE ) {
        //}
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

    static void handleScreenResize( 
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

            calculateProjectionMatrix( fbWidth, fbHeight, projMatrix );

            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
        }
    }
    
} // namespace


ApplicationDVR::ApplicationDVR(
    const GfxAPI::ContextOpenGL& contextOpenGL )
    : mContextOpenGL( contextOpenGL ) 
    , mDataFileUrl( "" )
    , mpData( nullptr )
    , mpDensityTex3d( nullptr )
    , mpNormalTex3d( nullptr )
    , mpProcess( nullptr )
    , mSharedMem( "DVR_shared_memory" )
    , mGrabCursor( true ) {

    mSharedMem.put( "histoBucketEntries", std::to_string( VolumeData::mNumHistogramBuckets ) );
    DVR_GUI::InitGui( contextOpenGL );
}

ApplicationDVR::~ApplicationDVR() {
    delete mpData;
    mpData = nullptr;
    
    delete mpDensityTex3d;
    mpDensityTex3d = nullptr;
    
    delete mpNormalTex3d;
    mpNormalTex3d = nullptr;
    
    if (mpProcess) { mpProcess->close_stdin(); mpProcess->kill(); int exitStatus = mpProcess->get_exit_status(); }
    delete mpProcess;
    mpProcess = nullptr;
}

Status_t ApplicationDVR::load( const std::string& fileUrl )
{
    mDataFileUrl = fileUrl;

    delete mpData;
    mpData = new VolumeData;
    mpData->load( fileUrl.c_str() );

    const auto volDim = mpData->getDim();
    const auto texDim = linAlg::i32vec3_t{ volDim[0], volDim[1], volDim[2] };
    GfxAPI::Texture::Desc_t volTexDesc{
        .texDim = texDim,
        .numChannels = 1,
        .channelType = GfxAPI::eChannelType::i16,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    delete mpDensityTex3d;

    mpDensityTex3d = new GfxAPI::Texture;
    mpDensityTex3d->create( volTexDesc );
    const uint32_t mipLvl = 0;
    mpDensityTex3d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );
    mpDensityTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
    mpDensityTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );
    mpDensityTex3d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 2 );

    mpData->calculateHistogramBuckets();
    const auto& histoBuckets = mpData->getHistoBuckets();

    mSharedMem.put( "volTexDim3D", reinterpret_cast<const uint8_t *const>( texDim.data() ), texDim.size() * sizeof( texDim[0] ) );
    mSharedMem.put( "histoBuckets", reinterpret_cast<const uint8_t *const>( histoBuckets.data() ), histoBuckets.size() * sizeof( histoBuckets[0] ) );
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

Status_t ApplicationDVR::run() {
    ArcBallControls arcBallControl;
    const ArcBall::ArcBallControls::InteractionModeDesc arcBallControlInteractionSettings{ .fullCircle = false, .smooth = false };

    
    // handle window resize
    int32_t fbWidth, fbHeight;
    glfwGetFramebufferSize( reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ), &fbWidth, &fbHeight);
    //printf( "glfwGetFramebufferSize: %d x %d\n", fbWidth, fbHeight );

    mScreenQuadHandle = gfxUtils::createScreenQuadGfxBuffers();

    // load shaders
    printf( "creating volume shader\n" ); fflush( stdout );
    GfxAPI::Shader volShader;
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > volShaderDesc{
        std::make_pair( "./src/shaders/tex3d-raycast.vert.glsl", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/tex3d-raycast.frag.glsl", GfxAPI::Shader::eShaderStage::FS ), // X-ray of x-y planes
    };
    gfxUtils::createShader( volShader, volShaderDesc );
    volShader.use( true );
    volShader.setInt( "u_densityTex", 0 );
    volShader.setFloat( "u_recipTexDim", 1.0f );
    volShader.use( false );
    
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

    glfwSetKeyCallback( pWindow, keyboardCallback );
    glfwSetScrollCallback( pWindow, mouseWheelCallback );
    
    glfwSetFramebufferSizeCallback( pWindow, framebufferSizeCallback );

    glCheckError();

    linAlg::vec3_t axis_mx{ 0.0f, 1.0f, 0.0f }; // initial model rotation
    linAlg::vec3_t axis_my{ 1.0f, 0.0f, 0.0f }; // initial model rotation

    float camZoomDist = 3.0f;
    float targetCamZoomDist = camZoomDist;
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


    StlModel stlModel;
    stlModel.load( "./src/data/blender-cube-ascii.STL" );
    //stlModel.load( "./data/blender-cube.STL" );

    
    mStlModelHandle = gfxUtils::createMeshGfxBuffers(
        stlModel.coords().size() / 3,
        stlModel.coords(),
        stlModel.normals().size() / 3,
        stlModel.normals(),
        stlModel.indices().size(),
        stlModel.indices() );
    GfxAPI::Shader meshShader;
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > meshShaderDesc{
        std::make_pair( "./src/shaders/rayMarchUnitCube.vert.glsl", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/rayMarchUnitCube.frag.glsl", GfxAPI::Shader::eShaderStage::FS ),
    };
    gfxUtils::createShader( meshShader, meshShaderDesc );
    meshShader.use( true );
    meshShader.setInt( "u_densityTex", 0 );
    meshShader.use( false );

    bool guiWantsMouseCapture = false;

    uint64_t frameNum = 0;
    while( !glfwWindowShouldClose( pWindow ) ) {

        const auto frameStartTime = std::chrono::system_clock::now();
        
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
            targetCamZoomDist += mouse_dy / ( fbHeight * mouseSensitivity * 0.5f );
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
                    linAlg::loadScaleMatrix( scaleMatrix, volDimRatio ); // unit cube approach
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

        //calculateFovYProjectionMatrix( fbWidth, fbHeight, fovY_deg, mProjMatrix );
        //linAlg::mat4_t tiltMat4;
        //linAlg::castMatrix( tiltMat4, tiltRotMat );
        ////mProjMatrix = tiltMat4 * mProjMatrix;
        //mProjMatrix = mProjMatrix * tiltMat4;

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

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glViewport( 0, 0, fbWidth, fbHeight ); // set to render-target size
        { // clear screen
            glBindFramebuffer( GL_FRAMEBUFFER, 0 );

            constexpr float clearColorValue[]{ 0.0f, 0.5f, 0.0f, 0.0f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
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


    #if 1 // unit-cube STL file
        if ( rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster ) {
            glEnable( GL_CULL_FACE );
            glCullFace( GL_FRONT );

            glBindVertexArray( mStlModelHandle.vaoHandle );
            meshShader.use( true );

            GfxAPI::Texture::unbindAllTextures();

            // linAlg::mat4_t mvpMatrix;
            // linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );
            auto retSetUniform = meshShader.setMat4( "u_mvpMat", mMvpMatrix );
            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->bindToTexUnit( 0 );
            }

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, mModelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformationToPoint( invModelViewMatrix, &camPos_OS, 1 );
            meshShader.setVec4( "u_camPos_OS", camPos_OS );
            meshShader.setVec3( "u_volDimRatio", volDimRatio );

            glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( stlModel.indices().size() ), GL_UNSIGNED_INT, 0 );

            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->unbindFromTexUnit();
            }

            meshShader.use( false );
            glBindVertexArray( 0 );
        }
    #endif


    #if 1 
        if ( rayMarchAlgo == DVR_GUI::eRayMarchAlgo::fullscreenBoxIsect ) {
            glDisable( GL_CULL_FACE );
            volShader.use( true );
            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->bindToTexUnit( 0 );
            }
            glBindVertexArray( mScreenQuadHandle.vaoHandle );

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, mModelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformationToPoint( invModelViewMatrix, &camPos_OS, 1 );
            volShader.setVec4( "u_camPos_OS", camPos_OS );
            volShader.setVec3( "u_volDimRatio", volDimRatio );

            //linAlg::mat4_t mvpMatrix;
            //linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );

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

            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->unbindFromTexUnit();
            }
            volShader.use( false );
        }
    #endif

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
            int rayMarchAlgoIdx = static_cast<int>(rayMarchAlgo);
            int* pRayMarchAlgoIdx = &rayMarchAlgoIdx;
            bool editTransferFunction = false;
            DVR_GUI::GuiUserData_t guiUserData{
                .volumeDataUrl = mDataFileUrl,
                .pRayMarchAlgoIdx = pRayMarchAlgoIdx,
                .loadFileTrigger = loadFileTrigger,
                .resetTrafos = resetTrafos,
                .wantsToCaptureMouse = guiWantsMouseCapture,
                .editTransferFunction = editTransferFunction,
            };

            DVR_GUI::DisplayGui( &guiUserData );

            if (*pRayMarchAlgoIdx != static_cast<int>(rayMarchAlgo)) {
                
                rayMarchAlgo = (DVR_GUI::eRayMarchAlgo)(*pRayMarchAlgoIdx);

                printf( "rayMarchAlgoIdx = %d, rayMarchAlgo = %d\n", *pRayMarchAlgoIdx, (int)rayMarchAlgo );
            }

            if (loadFileTrigger) {
                load( mDataFileUrl );
                glCheckError();
                const auto& minMaxDensity = mpData->getMinMaxDensity();
                volShader.use( true );
                volShader.setVec3( "u_minMaxScaleVal", linAlg::vec3_t{ static_cast<float>(minMaxDensity[0]) / 65535.0f, static_cast<float>(minMaxDensity[1]) / 65535.0f, 1.0f / ( static_cast<float>(minMaxDensity[1] - minMaxDensity[0]) / 65535.0f ) } );
                const auto volDim = mpData->getDim();
                const auto maxVolDim = linAlg::maximum( volDim[0], linAlg::maximum( volDim[1], volDim[2] ) );
                volShader.setFloat( "u_recipTexDim", 1.0f / linAlg::maximum( 1.0f, static_cast<float>(maxVolDim) ) );
                volShader.use( false );

                meshShader.use( true );
                meshShader.setVec3( "u_minMaxScaleVal", linAlg::vec3_t{ static_cast<float>(minMaxDensity[0]) / 65535.0f, static_cast<float>(minMaxDensity[1]) / 65535.0f, 1.0f / (static_cast<float>(minMaxDensity[1] - minMaxDensity[0]) / 65535.0f) } );
                meshShader.use( false );

                printf( "min max densities %u %u\n", (uint32_t)minMaxDensity[0], (uint32_t)minMaxDensity[1] );
                glCheckError();
                
                resetTransformations( arcBallControl, camTiltRadAngle, targetCamTiltRadAngle );

                loadFileTrigger = false;
            }

            if (resetTrafos) {
                printf( "reset Trafos!\n" );
                resetTransformations( arcBallControl, camTiltRadAngle, targetCamTiltRadAngle );
                resetTrafos = false;
            }
            
            if (guiUserData.editTransferFunction) {
                int exitStatus;
                if (mpProcess == nullptr || mpProcess->try_get_exit_status( exitStatus ) ) {
                    if (mpProcess) { mpProcess->kill(); }
                    delete mpProcess;
                    mpProcess = new TinyProcessLib::Process( mCmdLinePath );
                }
            }

        }

        //glfwWaitEventsTimeout( 0.032f ); // wait time is in seconds
        glfwSwapBuffers( pWindow );

        const auto frameEndTime = std::chrono::system_clock::now();
        frameDelta = static_cast<float>(std::chrono::duration_cast<std::chrono::duration<double>>(frameEndTime - frameStartTime).count());
        frameDelta = linAlg::minimum( frameDelta, 0.032f );

        targetCamZoomDist += mouseWheelOffset * zoomSpeed;
        targetCamZoomDist = linAlg::clamp( targetCamZoomDist, 0.1f, 1000.0f );
        camZoomDist = targetCamZoomDist * (1.0f - angleDamping) + camZoomDist * angleDamping;
        mouseWheelOffset = 0.0f;

        camTiltRadAngle = targetCamTiltRadAngle * (1.0f - angleDamping) + camTiltRadAngle * angleDamping;

        linAlg::scale( targetPanDeltaVector, (1.0f - panDamping) );

        prevMouseX = currMouseX;
        prevMouseY = currMouseY;

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

void ApplicationDVR::resetTransformations( ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle )
{
    arcBallControl.resetTrafos();
    camTiltRadAngle = 0.0f;
    // testTiltRadAngle = 0.0f;
    targetCamTiltRadAngle = 0.0f;
    panVector = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    targetPanDeltaVector = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    rotPivotPosOS = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    rotPivotPosWS = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    rotPivotOffset = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
    camZoomDist = 0.0f;
    targetCamZoomDist = initialCamZoomDist;
}
