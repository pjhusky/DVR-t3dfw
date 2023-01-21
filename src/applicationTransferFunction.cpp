#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <iostream>
#include <tchar.h>
#include <filesystem>

#include "ApplicationTransferFunction.h"

#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/checkErrorGL.h"

//#include "GUI/DVR_GUI.h"

//#include "external/simdb/simdb.hpp"

#include <memory>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#include <cassert>

#define IS_READY    0

namespace {

    constexpr float mouseSensitivity = 0.23f;
    static float frameDelta = 0.016f; // TODO: actually calculate frame duration in the main loop

    static const std::vector<TinyProcessLib::Process::string_type>& CommandLinePath( const char* const argv ) {
        static std::vector<TinyProcessLib::Process::string_type> cmdLinePath;

        //if (cmdLinePath.empty()) {
        //    std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
        //    auto argv0Wide = std::filesystem::_Convert_Source_to_wide( argv );
        //    cmdLinePath = std::vector<TinyProcessLib::Process::string_type>{
        //        //cmdLine.c_str(),
        //        argv0Wide,
        //        TinyProcessLib::Process::string_type{ _TEXT( "--colorPicker" ) },
        //        _T( "-x" ),
        //        _T( "600" ),
        //        _T( "-y" ),
        //        _T( "600" ) };
        //}

        return cmdLinePath;
    }

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

        //float camSpeed = camSpeedFact;
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_LEFT_SHIFT ) )  { camSpeed *= 5.0f; }
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_RIGHT_SHIFT ) ) { camSpeed *= 0.1f; }

        //if (pressedOrRepeat( pWindow, GLFW_KEY_W ))    { targetCamZoomDist -= camSpeed * frameDelta; }
        //if (pressedOrRepeat( pWindow, GLFW_KEY_S ))    { targetCamZoomDist += camSpeed * frameDelta; }

        //if (pressedOrRepeat( pWindow, GLFW_KEY_KP_6 )) { rotPivotOffset[0] += camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }
        //if (pressedOrRepeat( pWindow, GLFW_KEY_KP_4 )) { rotPivotOffset[0] -= camSpeed * frameDelta * pivotSpeed; printVec( "piv", rotPivotOffset ); }
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

    static void handleScreenResize( 
        GLFWwindow *const pWindow,
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

            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
        }
    }
    
} // namespace


ApplicationTransferFunction::ApplicationTransferFunction(
    const GfxAPI::ContextOpenGL& contextOpenGL )
    : mContextOpenGL( contextOpenGL ) 
    , mpDensityHistogramTex2d( nullptr )
    , mpProcess( nullptr )
    , mSharedMem( "DVR_shared_memory" )
    , mGrabCursor( true ) {



    GfxAPI::Texture::Desc_t texDesc{
        .texDim = linAlg::i32vec3_t{ 256, 256, 1 },
        .numChannels = 1,
        .channelType = GfxAPI::eChannelType::i16,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    delete mpDensityHistogramTex2d;

    //const uint32_t smBlockSizeInBytes = 1024u;
    //const uint32_t smNumBlocks = 4096u;
    //simdb sharedMem( "DVR_shared_memory", smBlockSizeInBytes, smNumBlocks );
    //const auto queriedSmVal = sharedMem.get( "lock free" );

    
    const auto queriedSmVal = mSharedMem.get( "lock free" );

    mpDensityHistogramTex2d = new GfxAPI::Texture;
    mpDensityHistogramTex2d->create( texDesc );
    const uint32_t mipLvl = 0;
#if ( IS_READY != 0 )
    mpDensityHistogramTex2d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );
#endif
    mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
    mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );

}

ApplicationTransferFunction::~ApplicationTransferFunction() {
    
    delete mpDensityHistogramTex2d;
    mpDensityHistogramTex2d = nullptr;
    
    if (mpProcess) { mpProcess->kill(); }
    delete mpProcess;
    mpProcess = nullptr;
}

Status_t ApplicationTransferFunction::load( const std::string& fileUrl )
{
#if ( IS_READY != 0 )
    mDataFileUrl = fileUrl;

    delete mpData;
    mpData = new VolumeData;
    mpData->load( fileUrl.c_str() );

    const auto volDim = mpData->getDim();
    GfxAPI::Texture::Desc_t volTexDesc{
        .texDim = linAlg::i32vec3_t{ volDim[0], volDim[1], volDim[2] },
        .numChannels = 1,
        .channelType = GfxAPI::eChannelType::i16,
        .semantics = GfxAPI::eSemantics::color,
        .isMipMapped = false,
    };
    delete mpDensityHistogramTex2d;

    mpDensityHistogramTex2d = new GfxAPI::Texture;
    mpDensityHistogramTex2d->create( volTexDesc );
    const uint32_t mipLvl = 0;
    mpDensityHistogramTex2d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );
    mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
    mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );
    mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 2 );
#endif
    return Status_t::OK();
}

Status_t ApplicationTransferFunction::run() {
    
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


    GfxAPI::Shader meshShader;
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > meshShaderDesc{
        std::make_pair( "./src/shaders/rayMarchUnitCube.vert.glsl", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/rayMarchUnitCube.frag.glsl", GfxAPI::Shader::eShaderStage::FS ),
    };
    gfxUtils::createShader( meshShader, meshShaderDesc );
    meshShader.use( true );
    meshShader.setInt( "u_densityTex", 0 );
    meshShader.use( false );

    //bool guiWantsMouseCapture = false;

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

        //if ( ( leftMouseButtonPressed && !guiWantsMouseCapture ) || rightMouseButtonPressed || middleMouseButtonPressed ) {
        //    if (mGrabCursor) { glfwSetInputMode( pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED ); }
        //    if (leftMouseButton_down == false && pressedOrRepeat( pWindow, GLFW_KEY_LEFT_CONTROL )) { // LMB was just freshly pressed
        //        setNewRotationPivot = 2;
        //    }
        //    leftMouseButton_down = true;
        //} else {
        //    glfwSetInputMode( pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
        //    leftMouseButton_down = false;
        //}


        if (rightMouseButtonPressed) {
            //printf( "RMB pressed!\n" );
            targetCamZoomDist += mouse_dy / ( fbHeight * mouseSensitivity * 0.5f );
            targetCamTiltRadAngle -= mouse_dx / (fbWidth * mouseSensitivity * 0.5f);
        }
        if (middleMouseButtonPressed) {
            //printf( "MMB pressed!\n" );
        }

        glCheckError();

        handleScreenResize(
            reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ),
            prevFbWidth,
            prevFbHeight,
            fbWidth,
            fbHeight );

        glfwGetFramebufferSize( reinterpret_cast<GLFWwindow*>(mContextOpenGL.window()), &fbWidth, &fbHeight );

        if (prevFbWidth != fbWidth || prevFbHeight != fbHeight) 
        {
            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
        }

        glCheckError();

        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glViewport( 0, 0, fbWidth, fbHeight ); // set to render-target size
        { // clear screen
            glBindFramebuffer( GL_FRAMEBUFFER, 0 );

            constexpr float clearColorValue[]{ 0.0f, 0.5f, 0.55f, 0.0f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
            //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        }

        
        const auto queriedSmVal = mSharedMem.get( "lock free" );

        mSharedMem.put( "from TF", "from TF value!" ); 

        if ( frameNum % 200 == 0 ) { printf( "in TF from SM for \lock free\": %s\n", queriedSmVal.c_str() ); }
        glCheckError();

    #if 0 // unit-cube STL file
        if ( rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster ) {
            glEnable( GL_CULL_FACE );
            glCullFace( GL_FRONT );

            glBindVertexArray( mStlModelHandle.vaoHandle );
            meshShader.use( true );

            GfxAPI::Texture::unbindAllTextures();

            // linAlg::mat4_t mvpMatrix;
            // linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );
            auto retSetUniform = meshShader.setMat4( "u_mvpMat", mMvpMatrix );
            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->bindToTexUnit( 0 );
            }

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, mModelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformationToPoint( invModelViewMatrix, &camPos_OS, 1 );
            meshShader.setVec4( "u_camPos_OS", camPos_OS );
            meshShader.setVec3( "u_volDimRatio", volDimRatio );

            glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( stlModel.indices().size() ), GL_UNSIGNED_INT, 0 );

            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->unbindFromTexUnit();
            }

            meshShader.use( false );
            glBindVertexArray( 0 );
        }
    #endif


    #if 0 
        if ( rayMarchAlgo == DVR_GUI::eRayMarchAlgo::fullscreenBoxIsect ) {
            glDisable( GL_CULL_FACE );
            volShader.use( true );
            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->bindToTexUnit( 0 );
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

            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->unbindFromTexUnit();
            }
            volShader.use( false );
        }
    #endif

        glCheckError();



        //glfwWaitEventsTimeout( 0.032f ); // wait time is in seconds
        glfwSwapBuffers( pWindow );

        const auto frameEndTime = std::chrono::system_clock::now();
        frameDelta = static_cast<float>(std::chrono::duration_cast<std::chrono::duration<double>>(frameEndTime - frameStartTime).count());
        frameDelta = linAlg::minimum( frameDelta, 0.032f );

        mouseWheelOffset = 0.0f;

        prevMouseX = currMouseX;
        prevMouseY = currMouseY;

        frameNum++;
    }

    //DVR_GUI::DestroyGui();

    #if 0
        std::stringstream commentStream;
        commentStream << "# grabbed framebuffer: " << 1 << "\n";
        constexpr bool ppmBinary = false;
        netBpmIO::writePpm( "grabbedFramebuffer.ppm", pGrabbedFramebuffer.get(), numSrcChannels, fbWidth, fbHeight, ppmBinary, commentStream.str() );
    #endif

    return Status_t::OK();
}
