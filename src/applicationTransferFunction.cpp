#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <iostream>
#include <tchar.h>
#include <filesystem>

#include "ApplicationTransferFunction.h"
#include "stringUtils.h"

#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/checkErrorGL.h"


#include "external/libtinyfiledialogs/tinyfiledialogs.h"


#include <memory>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#include <cassert>

#include "glad/glad.h"
#include "GLFW/glfw3.h"


#define IS_READY    0

namespace {

    constexpr float mouseSensitivity = 0.23f;
    static float frameDelta = 0.016f; // TODO: actually calculate frame duration in the main loop

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
    , mpDensityTransparencyTex2d( nullptr )
    , mpDensityColorsTex2d( nullptr )
    , mpDensityHistogramTex2d( nullptr )
    , mRelativeCoordY_DensityColors( 0.9f )
    , mpColorPickerProcess( nullptr )
    , mSharedMem( "DVR_shared_memory" )
    , mGrabCursor( true ) {

    printf( "begin ApplicationTransferFunction ctor\n" );

    mNumHistogramBuckets = stringUtils::convStrTo<uint32_t>( mSharedMem.get( "histoBucketEntries" ) );
    printf( "numHistogramBuckets: %u\n", mNumHistogramBuckets );

    
    mHistogramBuckets.resize( mNumHistogramBuckets );


    { // transparency
        GfxAPI::Texture::Desc_t texDesc{
            .texDim = linAlg::i32vec3_t{ 1024, 256, 0 },
            .numChannels = 1,
            .channelType = GfxAPI::eChannelType::i16,
            .semantics = GfxAPI::eSemantics::color,
            .isMipMapped = false,
        };
        delete mpDensityTransparencyTex2d;

        mpDensityTransparencyTex2d = new GfxAPI::Texture;
        mpDensityTransparencyTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
        mpDensityTransparencyTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
        mpDensityTransparencyTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );
    }

    { // colors
        GfxAPI::Texture::Desc_t texDesc{
            .texDim = linAlg::i32vec3_t{ 1024, 1, 0 },
            .numChannels = 4,
            .channelType = GfxAPI::eChannelType::i8,
            .semantics = GfxAPI::eSemantics::color,
            .isMipMapped = false,
        };
        delete mpDensityColorsTex2d;

        mpDensityColorsTex2d = new GfxAPI::Texture;
        mpDensityColorsTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
        mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
        mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToEdge, 1 );
    }

    { // histogram
        GfxAPI::Texture::Desc_t texDesc{
            .texDim = linAlg::i32vec3_t{ 1024, 256, 0 },
            .numChannels = 1,
            .channelType = GfxAPI::eChannelType::i8,
            .semantics = GfxAPI::eSemantics::color,
            .isMipMapped = false,
        };
        delete mpDensityHistogramTex2d;
    
        mpDensityHistogramTex2d = new GfxAPI::Texture;
        mpDensityHistogramTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
    #if ( IS_READY != 0 )
        mpDensityHistogramTex2d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );
    #endif
        mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
        mpDensityHistogramTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );
    }

    mDensityColors.insert( std::make_pair( 0,    linAlg::vec3_t{1.0f, 0.0f, 0.0f} ) );
    mDensityColors.insert( std::make_pair( 1023, linAlg::vec3_t{0.5f, 1.0f, 1.0f} ) );
    colorKeysToTex2d();

    printf( "end ApplicationTransferFunction ctor\n" );
}

ApplicationTransferFunction::~ApplicationTransferFunction() {
    
    printf( "begin ApplicationTransferFunction dtor\n" );

    delete mpDensityHistogramTex2d;
    mpDensityHistogramTex2d = nullptr;
    
    if (mpColorPickerProcess) { mpColorPickerProcess->close_stdin(); mpColorPickerProcess->kill(); int exitStatus = mpColorPickerProcess->get_exit_status(); }
    delete mpColorPickerProcess;
    mpColorPickerProcess = nullptr;

    printf( "end ApplicationTransferFunction dtor\n" );
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


    GfxAPI::Shader shader;
    std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > meshShaderDesc{
        std::make_pair( "./src/shaders/displayColors.vert.glsl", GfxAPI::Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/displayColors.frag.glsl", GfxAPI::Shader::eShaderStage::FS ),
    };
    gfxUtils::createShader( shader, meshShaderDesc );
    shader.use( true );
    shader.setInt( "u_mapTex", 0 );
    shader.use( false );

    linAlg::i32vec3_t texDim{ 0, 0 , 0 };

    //bool guiWantsMouseCapture = false;
    linAlg::vec3_t clearColor{ 0.0f, 0.5f, 0.55f };
    uint64_t frameNum = 0;
    while( !glfwWindowShouldClose( pWindow ) ) {

        const auto frameStartTime = std::chrono::system_clock::now();
        
        if ( frameNum % 5 == 0 && mSharedMem.get( "histoBucketsDirty" ) == "true" ) {
            
            uint32_t bytesRead = 0;

            const auto resultVolTexDim3D = mSharedMem.get( "volTexDim3D", texDim.data(), texDim.size() * sizeof( texDim[0] ), &bytesRead );
            if ( resultVolTexDim3D == true ) {
                assert( bytesRead == texDim.size() * sizeof( texDim[0] ) );
                printf( "shared mem read tex dim %d %d %d\n", texDim[0], texDim[1], texDim[2] );
            }

            const auto resultHistoBuckets = mSharedMem.get( "histoBuckets", mHistogramBuckets.data(), mHistogramBuckets.size() * sizeof( uint32_t ), &bytesRead );
            if ( resultHistoBuckets == true ) {
                assert( bytesRead == mHistogramBuckets.size() * sizeof( uint32_t ) );
            }

            densityHistogramToTex2d();

            mSharedMem.put( "histoBucketsDirty", "false" );
            printf( "resultHistoBuckets = %s\n", resultHistoBuckets ? "true" : "false" );
        }
        
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

        if ( leftMouseButtonPressed && frameNum > 4 ) {
            printf( "appTF: LMB pressed\n" );
        #if 1
            if ( currMouseY > mRelativeCoordY_DensityColors * fbHeight ) {
                unsigned char lRgbColor[3]{ clearColor[0] * 255.0f, clearColor[1] * 255.0f, clearColor[2] * 255.0f };
                auto lTheHexColor = tinyfd_colorChooser(
                    "Choose Transfer-function Color",
                    "#FF0077",
                    lRgbColor,
                    lRgbColor);
                if (lTheHexColor) {
                    clearColor[0] = ( 1.0f / 255.0f ) * lRgbColor[ 0 ];
                    clearColor[1] = ( 1.0f / 255.0f ) * lRgbColor[ 1 ];
                    clearColor[2] = ( 1.0f / 255.0f ) * lRgbColor[ 2 ];
                }
            }
        #else
            printf( "appTF: mpColorPickerProcess is nullptr? %s\n", ( mpColorPickerProcess == nullptr ) ? "yes" : "no" );
            int exitStatus;
            if (mpColorPickerProcess == nullptr || mpColorPickerProcess->try_get_exit_status( exitStatus ) ) {
                printf( "appTF: LMB pressed and spinning up process!\n" );
                if (mpColorPickerProcess != nullptr) { 
                    mpColorPickerProcess->kill(); 
                    delete mpColorPickerProcess;
                    mpColorPickerProcess = nullptr;
                }
                printf( "appTF: LMB pressed, look at the supplied args:\n" );
                int32_t i = 0;
                for ( const auto& cmdArg : mCmdLineColorPickerProcess ) {
                    //wprintf( "%d: %s\n", i, cmdArg.c_str() );
                    std::wcout << i << " " << cmdArg << std::endl;
                    i++;
                }
                mpColorPickerProcess = new TinyProcessLib::Process( mCmdLineColorPickerProcess );
            }
        #endif
        }

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

            const float clearColorValue[]{ clearColor[0], clearColor[1], clearColor[2], 0.0f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
            //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        }

        
        const auto queriedSmVal = mSharedMem.get( "lock free" );

        mSharedMem.put( "from TF", "from TF value!" ); 

        //if ( frameNum % 200 == 0 ) { printf( "in TF from SM for \"lock free\": %s\n", queriedSmVal.c_str() ); }

        glCheckError();

        glDisable( GL_CULL_FACE );
        glBindVertexArray( mScreenQuadHandle.vaoHandle );
        shader.use( true );

        GfxAPI::Texture::unbindAllTextures();

        //if (mpDensityHistogramTex2d != nullptr) {
        //    mpDensityHistogramTex2d->bindToTexUnit( 0 );
        //}

        mpDensityColorsTex2d->bindToTexUnit( 0 );
        shader.setInt( "u_mapTex", 0 );
        shader.setVec2( "u_scaleOffset", GfxAPI::Shader::vec2_t{ 1.0f - mRelativeCoordY_DensityColors, mRelativeCoordY_DensityColors } );
        glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );


        mpDensityHistogramTex2d->bindToTexUnit( 1 );
        shader.setInt( "u_mapTex", 1 );
        shader.setVec2( "u_scaleOffset", GfxAPI::Shader::vec2_t{ 0.4f, 0.5f } );
        glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );

        if ( mpDensityColorsTex2d != nullptr ) {
            mpDensityColorsTex2d->unbindFromTexUnit();
        }
        if (mpDensityHistogramTex2d != nullptr) {
            mpDensityHistogramTex2d->unbindFromTexUnit();
        }

        shader.use( false );
        glBindVertexArray( 0 );


    #if 0 // unit-cube STL file
        if ( rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster ) {
            glEnable( GL_CULL_FACE );
            glCullFace( GL_FRONT );

            glBindVertexArray( mStlModelHandle.vaoHandle );
            shader.use( true );

            GfxAPI::Texture::unbindAllTextures();

            // linAlg::mat4_t mvpMatrix;
            // linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );
            auto retSetUniform = shader.setMat4( "u_mvpMat", mMvpMatrix );
            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->bindToTexUnit( 0 );
            }

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, mModelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformationToPoint( invModelViewMatrix, &camPos_OS, 1 );
            shader.setVec4( "u_camPos_OS", camPos_OS );
            shader.setVec3( "u_volDimRatio", volDimRatio );

            glDrawElements( GL_TRIANGLES, static_cast<GLsizei>( stlModel.indices().size() ), GL_UNSIGNED_INT, 0 );

            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->unbindFromTexUnit();
            }

            shader.use( false );
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

void ApplicationTransferFunction::colorKeysToTex2d() {
    std::array<uint8_t, 1024 * 4> interpolatedDataCPU;
    auto currLeft = mDensityColors.begin();
    auto currRight = mDensityColors.begin();
    for ( ++currRight; currRight != mDensityColors.end(); currRight++ ) {
         const uint32_t curr_x_R = currRight->first;

         const uint32_t startX = currLeft->first;
         const linAlg::vec3_t startColor = currLeft->second;

         const uint32_t endX   = currRight->first;
         const linAlg::vec3_t endColor = currRight->second;

         for ( uint32_t idx = startX; idx <= endX; idx++ ) {
             float fx = static_cast<float>(idx - startX) / static_cast<float>(endX - startX);
             const auto mixRGB = startColor * (1.0f-fx) + endColor * fx;
             //interpolatedDataCPU[idx] = 
             //    ( static_cast<uint32_t>( mixRGB[0] * 255.0f ) << 24u ) |
             //    ( static_cast<uint32_t>( mixRGB[1] * 255.0f ) << 16u ) |
             //    ( static_cast<uint32_t>( mixRGB[2] * 255.0f ) <<  8u ) | 255u;

             //interpolatedDataCPU[idx] = 0xFFFFFFFF;
             //interpolatedDataCPU[idx * 4 + 0] = 0x6F;
             //interpolatedDataCPU[idx * 4 + 1] = 0x6F;
             //interpolatedDataCPU[idx * 4 + 2] = 0x6F;
             //interpolatedDataCPU[idx * 4 + 3] = 0x6F;

             interpolatedDataCPU[idx * 4 + 0] = static_cast<uint8_t>( mixRGB[0] * 255.0f );
             interpolatedDataCPU[idx * 4 + 1] = static_cast<uint8_t>( mixRGB[1] * 255.0f );
             interpolatedDataCPU[idx * 4 + 2] = static_cast<uint8_t>( mixRGB[2] * 255.0f );
             interpolatedDataCPU[idx * 4 + 3] = 255u;

         }
         //for ( float fx = 0.0f; fx <= 1.0f; fx += 1.0f / (endX - startX + 1) ) {

         //    const auto mixRGB = startColor * (1.0f-fx) + endColor * fx;

         //    /*interpolatedDataCPU[startX+idx] = 
         //        ( static_cast<uint32_t>( mixRGB[0] * 255.0f ) << 24u ) |
         //        ( static_cast<uint32_t>( mixRGB[1] * 255.0f ) << 16u ) |
         //        ( static_cast<uint32_t>( mixRGB[2] * 255.0f ) <<  8u ) | 255u;*/

         //    interpolatedDataCPU[startX+idx] = 0xFFFFFFFF;
         //    idx++;
         //}

         currLeft = currRight;
    }

    //printf( "같같 ----- 같같\n" );
    mpDensityColorsTex2d->uploadData( interpolatedDataCPU.data(), GL_RGBA, GL_UNSIGNED_BYTE, 0 );
}

void ApplicationTransferFunction::densityHistogramToTex2d() {
    std::vector<uint8_t> densityHistogramCPU;
    const uint32_t dim_x = mpDensityHistogramTex2d->desc().texDim[0];
    const uint32_t dim_y = mpDensityHistogramTex2d->desc().texDim[1];
    densityHistogramCPU.resize( dim_x * dim_y );
    std::fill( densityHistogramCPU.begin(), densityHistogramCPU.end(), 0 );

    printf( "1\n" );

    uint32_t maxHistoVal = 1;
    for( uint32_t x = 30; x < mNumHistogramBuckets; x++ ) {
        maxHistoVal = linAlg::maximum( maxHistoVal, mHistogramBuckets[x] );
    }
    const float fRecipMaxHistoVal = 1.0f / static_cast<float>( maxHistoVal );

    printf( "2\n" );

    for( uint32_t x = 0; x < dim_x; x++ ) {
        const auto histoVal = mHistogramBuckets[x];
        const float relativeHistoH = histoVal * fRecipMaxHistoVal;
        for ( uint32_t y = 0; y < static_cast<uint32_t>(relativeHistoH * dim_y); y++ ) {
            if ( y >= dim_y ) { break; }
            densityHistogramCPU[ y * dim_x + x ] = 255;
        }
    }
    printf( "3\n" );
    mpDensityHistogramTex2d->uploadData( densityHistogramCPU.data(), GL_RED, GL_UNSIGNED_BYTE, 0 );
}
