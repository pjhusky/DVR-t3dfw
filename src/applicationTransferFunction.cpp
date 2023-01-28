#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <iostream>
#include <tchar.h>
#include <filesystem>

#include "applicationTransferFunction.h"
#include "stringUtils.h"

#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/checkErrorGL.h"

#include "fileLoaders/stb/stb_image.h"


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
    static constexpr uint32_t minRelevantDensity = 40u;
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
    , mpDensityTransparenciesTex2d( nullptr )
    , mpDensityColorsTex2d( nullptr )
    , mpDensityColorDotTex2d( nullptr )
    , mpDensityHistogramTex2d( nullptr )
    , mpColorPickerProcess( nullptr )
    , mSharedMem( "DVR_shared_memory" )
    , mGrabCursor( true ) {

    printf( "begin ApplicationTransferFunction ctor\n" );

    mScaleAndOffset_Transparencies  = { 1.0f, 0.8f, 0.0, 0.0f };
    mScaleAndOffset_Colors          = { 1.0f, 0.2f, 0.0, 0.8f };

    mNumHistogramBuckets = stringUtils::convStrTo<uint32_t>( mSharedMem.get( "histoBucketEntries" ) );
    printf( "numHistogramBuckets: %u\n", mNumHistogramBuckets );

    
    mHistogramBuckets.resize( mNumHistogramBuckets );


    { // transparency
        const GfxAPI::Texture::Desc_t texDesc{
            .texDim = linAlg::i32vec3_t{ ApplicationDVR_common::numDensityBuckets, 256, 0 },
            .numChannels = 2,
            .channelType = GfxAPI::eChannelType::i8,
            .semantics = GfxAPI::eSemantics::color,
            .isMipMapped = false,
        };
        delete mpDensityTransparenciesTex2d;

        mpDensityTransparenciesTex2d = new GfxAPI::Texture;
        mpDensityTransparenciesTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
        mpDensityTransparenciesTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
        mpDensityTransparenciesTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );

        mTransparencyPaintHeightsCPU.resize( texDesc.texDim[0] );

        { // transparencies

            uint32_t bytesRead;
            bool foundTransparencies = mSharedMem.get( "TransparencyPaintHeightsCPU", mTransparencyPaintHeightsCPU.data(), static_cast<uint32_t>( mTransparencyPaintHeightsCPU.size() ), &bytesRead );
            //if ( !foundTransparencies ) {// linear ramp
            //    const float conversionFactor = 1.0f / static_cast<float>( mTransparencyPaintHeightsCPU.size() );
            //    for ( int32_t idx = 0; idx < mTransparencyPaintHeightsCPU.size(); idx++ ) {
            //        mTransparencyPaintHeightsCPU[idx] = static_cast<uint8_t>( 255.0f * static_cast<float>(idx) * conversionFactor );
            //    }
            //}
            const uint32_t lowerBound = static_cast<uint32_t>(mTransparencyPaintHeightsCPU.size() * 15 / 100);
            const uint32_t upperBound = static_cast<uint32_t>(mTransparencyPaintHeightsCPU.size() * 25 / 100);
            if (!foundTransparencies) {// linear ramp
                for (uint32_t idx = 0; idx < lowerBound; idx++) {
                    mTransparencyPaintHeightsCPU[idx] = 0;
                }
                for (uint32_t idx = lowerBound; idx < upperBound; idx++) {
                    mTransparencyPaintHeightsCPU[idx] = 10u;
                }
                for (uint32_t idx = upperBound; idx < mTransparencyPaintHeightsCPU.size(); idx++) {
                    mTransparencyPaintHeightsCPU[idx] = 0;
                }
            }
            densityTransparenciesToTex2d();

            mSharedMem.put( "TransparencyPaintHeightsCPU", mTransparencyPaintHeightsCPU.data(), static_cast<uint32_t>( mTransparencyPaintHeightsCPU.size() ) );
            mSharedMem.put( "TFdirty", "true" );
        }

    }

    { // histogram
        const GfxAPI::Texture::Desc_t texDesc{
            .texDim = linAlg::i32vec3_t{ ApplicationDVR_common::numDensityBuckets, 256, 0 },
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

    { // colors
        const GfxAPI::Texture::Desc_t texDesc = ApplicationDVR_common::densityColorsTexDesc();
        delete mpDensityColorsTex2d;

        mpDensityColorsTex2d = new GfxAPI::Texture;
        mpDensityColorsTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
        mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
        mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToEdge, 1 );
        //mpDensityColorsTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 1 );
    }

    { // color dot
        GfxAPI::Texture::Desc_t texDesc = ApplicationDVR_common::densityColorsTexDesc();
        delete mpDensityColorDotTex2d;

        stbi_set_flip_vertically_on_load( 1 );
        int32_t imgNumChannels;
        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // needed for RGB images with odd width
        //uint8_t* pData = stbi_load( "data/colorDot_biggerCutOut_512_transparent_blurred.png", &texDesc.texDim[0], &texDesc.texDim[1], &texDesc.numChannels, 0 );
        uint8_t* pData = stbi_load( "data/colorDot_biggerCutOut_512_transparent.png", &texDesc.texDim[0], &texDesc.texDim[1], &texDesc.numChannels, 0 );


        mpDensityColorDotTex2d = new GfxAPI::Texture;
        mpDensityColorDotTex2d->create( texDesc );
        const uint32_t mipLvl = 0;
        mpDensityColorDotTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clamp, 0 );
        mpDensityColorDotTex2d->setWrapModeForDimension( GfxAPI::eBorderMode::clampToEdge, 1 );


        mpDensityColorDotTex2d->uploadData( pData, GL_RGBA, GL_UNSIGNED_BYTE, mipLvl );

        //gfxUtils::createTexFromImage( "data/colorDot_biggerCutOut_512_transparent_blurred.png", 
    }
    

    mDensityColors.clear();

    //mDensityColors.insert( std::make_pair( 0,    linAlg::vec3_t{1.0f, 0.0f, 0.0f} ) );
    //mDensityColors.insert( std::make_pair( 1023, linAlg::vec3_t{0.5f, 1.0f, 1.0f} ) );

    mDensityColors.insert( std::make_pair(    0, linAlg::vec3_t{0.0f, 0.0f, 0.0f} ) );
    mDensityColors.insert( std::make_pair(   60, linAlg::vec3_t{1.0f, 0.0f, 0.0f} ) );
    //mDensityColors.insert( std::make_pair(  250, linAlg::vec3_t{0.0f, 0.3f, 0.5f} ) );
    mDensityColors.insert( std::make_pair(  200, linAlg::vec3_t{0.0f, 0.3f, 0.5f} ) );
    mDensityColors.insert( std::make_pair(  300, linAlg::vec3_t{0.1f, 0.5f, 0.3f} ) );
    //mDensityColors.insert( std::make_pair( 200, linAlg::vec3_t{0.5f, 1.0f, 1.0f} ) );
    mDensityColors.insert( std::make_pair( 1023, linAlg::vec3_t{0.5f, 1.0f, 1.0f} ) );
    //mDensityColors.insert( std::make_pair( 1023, linAlg::vec3_t{1.0f, 1.0f, 1.0f} ) );

    //mDensityColors.insert( std::make_pair( 0,    linAlg::vec3_t{0.5f, 0.5f, 0.5f} ) );
    //mDensityColors.insert( std::make_pair( 1023, linAlg::vec3_t{0.5f, 0.5f, 0.5f} ) );

    colorKeysToTex2d();
    mSharedMem.put( "TFdirty", "true" );

    {
        std::vector<uint8_t> densityHistogramCPU;
        const uint32_t dim_x = mpDensityHistogramTex2d->desc().texDim[0];
        const uint32_t dim_y = mpDensityHistogramTex2d->desc().texDim[1];
        densityHistogramCPU.resize( dim_x * dim_y );
        std::fill( densityHistogramCPU.begin(), densityHistogramCPU.end(), 255 );
        mpDensityHistogramTex2d->uploadData( densityHistogramCPU.data(), GL_RED, GL_UNSIGNED_BYTE, 0 );
    }

    printf( "end ApplicationTransferFunction ctor\n" );
}

ApplicationTransferFunction::~ApplicationTransferFunction() {
    
    printf( "begin ApplicationTransferFunction dtor\n" );

    delete mpDensityHistogramTex2d;
    mpDensityHistogramTex2d = nullptr;
    
    delete mpDensityTransparenciesTex2d;
    mpDensityTransparenciesTex2d = nullptr;

    delete mpDensityColorsTex2d;
    mpDensityColorsTex2d = nullptr;


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

    glDisable( GL_DEPTH_TEST );

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

    //bool leftMouseButton_down = false;

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
    shader.setInt( "u_mode", 0 );
    shader.use( false );

    linAlg::i32vec3_t texDim{ 0, 0 , 0 };
    bool inTransparencyInteractionMode = false;
    bool inColorInteractionMode = false;
    float distMouseMovementWhileInColorInteractionMode = 0.0f;
    
    bool prevLeftMouseButtonPressed = false;
    uint32_t lockedBucketIdx = std::numeric_limits<uint32_t>::max(); //  static_cast<uint32_t>(-1);

    //bool guiWantsMouseCapture = false;
    //linAlg::vec3_t clearColor{ 0.0f, 0.5f, 0.55f };
    linAlg::vec3_t clearColor{ 0.0f, 0.0f, 0.0f };
    uint64_t frameNum = 0;
    uint64_t parentProcessWatchdogTicks = 0;
    while( !glfwWindowShouldClose( pWindow ) ) {

        const auto frameStartTime = std::chrono::system_clock::now();
        
        //std::time_t dvrTime = stringUtils::convStrTo<std::time_t>( mSharedMem.get( "DVR-app-time" ) );
        //if ( std::chrono::system_clock::to_time_t(frameStartTime) - dvrTime > 5 ) { 
        //    printf( "WATCHDOG time - haven't heard from DVR app for more than 5sec - bailing out!\n" ); 
        //}

        if ( mSharedMem.get( "stopTransferFunctionApp" ) == "true"  ) { break; }

        if (mSharedMem.get( "DVR_TF_tick" ) == "1") {
            mSharedMem.put( "DVR_TF_tick", "0" );
            parentProcessWatchdogTicks = 0;
        } else {
            parentProcessWatchdogTicks++;
        }

        if (parentProcessWatchdogTicks > 180) {
            printf( "watchdog timer expired, parent process most probably doesn't exist anymore!\n" );
            break;
        }

        if ( /*frameNum % 3 == 0 &&*/ mSharedMem.get( "histoBucketsDirty" ) == "true" ) {
            
            uint32_t bytesRead = 0;

            const auto resultVolTexDim3D = mSharedMem.get( "volTexDim3D", texDim.data(), static_cast<uint32_t>( texDim.size() * sizeof( texDim[0] ) ), &bytesRead );
            if ( resultVolTexDim3D == true ) {
                assert( bytesRead == texDim.size() * sizeof( texDim[0] ) );
                printf( "shared mem read tex dim %d %d %d\n", texDim[0], texDim[1], texDim[2] );
            }

            const auto resultHistoBuckets = mSharedMem.get( "histoBuckets", mHistogramBuckets.data(), static_cast<uint32_t>( mHistogramBuckets.size() * sizeof( uint32_t ) ), &bytesRead );
            if ( resultHistoBuckets == true ) {
                assert( bytesRead == mHistogramBuckets.size() * sizeof( uint32_t ) );
            }

            densityHistogramToTex2d();

            { // transparencies                
                uint32_t bytesRead;
                bool foundTransparencies = mSharedMem.get( "TransparencyPaintHeightsCPU", mTransparencyPaintHeightsCPU.data(), static_cast<uint32_t>( mTransparencyPaintHeightsCPU.size() ), &bytesRead );
                if ( !foundTransparencies ) {// linear ramp
                    const float conversionFactor = 1.0f / static_cast<float>( mTransparencyPaintHeightsCPU.size() );
                    for ( int32_t idx = 0; idx < mTransparencyPaintHeightsCPU.size(); idx++ ) {
                        mTransparencyPaintHeightsCPU[idx] = static_cast<uint8_t>( 255.0f * static_cast<float>(idx) * conversionFactor );
                    }
                }
                densityTransparenciesToTex2d();
            }


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

        bool leftMouseButtonJustReleased = ( !leftMouseButtonPressed && prevLeftMouseButtonPressed );

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

        if ( !leftMouseButtonPressed ) { 
            inTransparencyInteractionMode = false; 
        }

        const int32_t maxDeviationX_colorDots = 5;
        constexpr uint32_t startIdleFrames = 4;

        if ( leftMouseButtonPressed && frameNum > startIdleFrames ) {
            //printf( "appTF: LMB pressed\n" );
        #if 1
            const float maxY_transparencies = ( mScaleAndOffset_Transparencies[1] + mScaleAndOffset_Transparencies[3] ) * fbHeight;
            if ( !inColorInteractionMode && ( currMouseY < maxY_transparencies || inTransparencyInteractionMode ) ) {
                // clicked into the density-to-transparency & density histogram window
                inTransparencyInteractionMode = true;

                currMouseY = linAlg::clamp( currMouseY, 0.0f, maxY_transparencies - 1.0f );

                float relMouseStartX = currMouseX / fbWidth;
                float relMouseEndX   = prevMouseX / fbWidth;
                if ( relMouseEndX < relMouseStartX ) { std::swap( relMouseStartX, relMouseEndX ); }

                const float relMouseY = currMouseY / maxY_transparencies;

                const uint32_t texStartX = static_cast<uint32_t>( relMouseStartX * ( mpDensityTransparenciesTex2d->desc().texDim[0] - 1 ) );
                const uint32_t texEndX   = static_cast<uint32_t>( relMouseEndX   * ( mpDensityTransparenciesTex2d->desc().texDim[0] - 1 ) );
                const uint32_t texY = static_cast<uint32_t>( (1.0f - relMouseY ) * ( mpDensityTransparenciesTex2d->desc().texDim[1] - 1 ) );

                const int32_t kernelSize = 2;
                for (   int32_t x = linAlg::maximum<int32_t>( texStartX - kernelSize, 0 ); 
                        x < linAlg::minimum<int32_t>( texEndX + kernelSize, mpDensityTransparenciesTex2d->desc().texDim[0] ); 
                        x++ ) {
                    mTransparencyPaintHeightsCPU[x] = texY;
                }

                densityTransparenciesToTex2d();

                mSharedMem.put( "TransparencyPaintHeightsCPU", mTransparencyPaintHeightsCPU.data(), static_cast<uint32_t>( mTransparencyPaintHeightsCPU.size() ) );
                


                colorKeysToTex2d(); // uses the updated transparencies for the alpha value!

                
                mSharedMem.put( "TFdirty", "true" );

            } else if ( !inTransparencyInteractionMode && ( currMouseY > mScaleAndOffset_Colors[3] * fbHeight || inColorInteractionMode ) ) {
                // clicked into the density-to-color window (color picker)
                

                const auto densityBucketIdx = static_cast<int32_t>( ( currMouseX * ApplicationDVR_common::numDensityBuckets ) / (fbWidth - 1) + 0.5f );

                if (inColorInteractionMode) {
                    distMouseMovementWhileInColorInteractionMode += mouse_dx;

                    mDensityColors.erase( lockedBucketIdx );
                    lockedBucketIdx = densityBucketIdx;
                    mDensityColors.insert( std::make_pair( densityBucketIdx, clearColor ) );

                    colorKeysToTex2d();
                    mSharedMem.put( "TFdirty", "true" );
                } else {

                    decltype(mDensityColors)::iterator result = mDensityColors.end();
                    for (uint32_t xWithDeviation = linAlg::maximum( densityBucketIdx - maxDeviationX_colorDots, 0 );
                        xWithDeviation < linAlg::minimum( densityBucketIdx + maxDeviationX_colorDots, fbWidth );
                        xWithDeviation++) {
                        result = mDensityColors.find( xWithDeviation );
                        if (result != mDensityColors.end()) {
                            clearColor = result->second;
                            lockedBucketIdx = xWithDeviation;
                            //mDensityColors.erase( xWithDeviation );
                            if (result->first != 0 && result->first != 1023) { // TODO: handle edges left and right
                                inColorInteractionMode = true;
                            }
                            break;
                        }
                    }

                    if (result == mDensityColors.end()) { // no existing color dot clicked
                        mDensityColors.insert( std::make_pair( densityBucketIdx, clearColor ) );
                        result = mDensityColors.find( densityBucketIdx );

                        uint8_t lRgbColor[3]{
                            static_cast<uint8_t>(clearColor[0] * 255.0f),
                            static_cast<uint8_t>(clearColor[1] * 255.0f),
                            static_cast<uint8_t>(clearColor[2] * 255.0f) };
                        auto lTheHexColor = tinyfd_colorChooser(
                            "Choose Transfer-function Color",
                            nullptr,
                            lRgbColor,
                            lRgbColor );
                        if (lTheHexColor) {
                            clearColor[0] = (1.0f / 255.0f) * lRgbColor[0];
                            clearColor[1] = (1.0f / 255.0f) * lRgbColor[1];
                            clearColor[2] = (1.0f / 255.0f) * lRgbColor[2];

                            if (result != mDensityColors.end()) {
                                result->second = clearColor;
                            }
                        }
                        colorKeysToTex2d();
                        mSharedMem.put( "TFdirty", "true" );
                    }
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

        if (leftMouseButtonJustReleased) {
            if (inColorInteractionMode) {
                //const auto densityBucketIdx = static_cast<int32_t>((currMouseX * ApplicationDVR_common::numDensityBuckets) / (fbWidth - 1) + 0.5f);
                if (distMouseMovementWhileInColorInteractionMode < maxDeviationX_colorDots) {
                    decltype(mDensityColors)::iterator result = mDensityColors.end();
                    //mDensityColors.erase( lockedBucketIdx );
                    //lockedBucketIdx = densityBucketIdx;
                    //mDensityColors.insert( std::make_pair( densityBucketIdx, clearColor ) );
                    result = mDensityColors.find( lockedBucketIdx );

                    uint8_t lRgbColor[3]{
                        static_cast<uint8_t>(clearColor[0] * 255.0f),
                        static_cast<uint8_t>(clearColor[1] * 255.0f),
                        static_cast<uint8_t>(clearColor[2] * 255.0f) };
                    auto lTheHexColor = tinyfd_colorChooser(
                        "Choose Transfer-function Color",
                        nullptr,
                        lRgbColor,
                        lRgbColor );
                    if (lTheHexColor) {
                        clearColor[0] = (1.0f / 255.0f) * lRgbColor[0];
                        clearColor[1] = (1.0f / 255.0f) * lRgbColor[1];
                        clearColor[2] = (1.0f / 255.0f) * lRgbColor[2];

                        if (result != mDensityColors.end()) {
                            result->second = clearColor;
                        }
                    }
                    colorKeysToTex2d();
                    mSharedMem.put( "TFdirty", "true" );

                }
            }
            //lockedBucketIdx = std::numeric_limits<uint32_t>::max();
            inColorInteractionMode = false;
            distMouseMovementWhileInColorInteractionMode = 0.0f;
        }

        if (rightMouseButtonPressed) {
            //printf( "RMB pressed!\n" );
            targetCamZoomDist += mouse_dy / ( fbHeight * mouseSensitivity * 0.5f );
            targetCamTiltRadAngle -= mouse_dx / (fbWidth * mouseSensitivity * 0.5f);

            if (frameNum > startIdleFrames) { // check if existing color dot was right-clicked and if yes erase that color dot
                const auto densityBucketIdx = static_cast<int32_t>((currMouseX * ApplicationDVR_common::numDensityBuckets) / (fbWidth - 1) + 0.5f);

                decltype(mDensityColors)::iterator result = mDensityColors.end();
                for (uint32_t xWithDeviation = linAlg::maximum( densityBucketIdx - maxDeviationX_colorDots, 0 );
                    xWithDeviation < linAlg::minimum( densityBucketIdx + maxDeviationX_colorDots, fbWidth );
                    xWithDeviation++) {
                    result = mDensityColors.find( xWithDeviation );
                    if (result != mDensityColors.end()) {
                        mDensityColors.erase( xWithDeviation );
                        colorKeysToTex2d();
                        mSharedMem.put( "TFdirty", "true" );
                        break;
                    }
                }
            }
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

        
        
        glViewport( 0, 0, fbWidth, fbHeight ); // set to render-target size
        { // clear screen
            glBindFramebuffer( GL_FRAMEBUFFER, 0 );

            const float clearColorValue[]{ clearColor[0], clearColor[1], clearColor[2], 0.0f };
            //const float clearColorValue[]{ clearColor[0], clearColor[1], clearColor[2], 1.0f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
            //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        }

        
        //if ( frameNum % 200 == 0 ) { printf( "in TF from SM for \"lock free\": %s\n", queriedSmVal.c_str() ); }

        glCheckError();

        glDisable( GL_CULL_FACE );
        glBindVertexArray( mScreenQuadHandle.vaoHandle );
        shader.use( true );

        GfxAPI::Texture::unbindAllTextures();

        //if (mpDensityHistogramTex2d != nullptr) {
        //    mpDensityHistogramTex2d->bindToTexUnit( 0 );
        //}

        shader.setInt( "u_mode", 0 );

        if ( mpDensityTransparenciesTex2d != nullptr ) {
            mpDensityTransparenciesTex2d->bindToTexUnit( 0 );
            shader.setInt( "u_mapTex", 0 );
            shader.setVec4( "u_scaleOffset", mScaleAndOffset_Transparencies );

            if (mpDensityHistogramTex2d != nullptr) {
                mpDensityHistogramTex2d->bindToTexUnit( 1 );
                shader.setInt( "u_bgTex", 1 );
            }
            shader.setInt( "u_mode", 1 );

            glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );
            mpDensityTransparenciesTex2d->unbindFromTexUnit();
        }

        shader.setInt( "u_mode", 0 );

        if ( mpDensityColorsTex2d != nullptr ) {
            mpDensityColorsTex2d->bindToTexUnit( 2 );
            shader.setInt( "u_mapTex", 2 );
            shader.setVec4( "u_scaleOffset", mScaleAndOffset_Colors );
            glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );        
            mpDensityColorsTex2d->unbindFromTexUnit();
        }

    #if 1
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    #endif
        if (mpDensityColorDotTex2d != nullptr) {
            mpDensityColorDotTex2d->bindToTexUnit( 3 );
            shader.setInt( "u_mapTex", 3 );

            const auto screenRatio = static_cast<float>(fbHeight) / fbWidth;
            linAlg::vec4_t scaleAndOffset{ 0.02f * screenRatio, 0.02f, 0.0f, mScaleAndOffset_Colors[3] + mScaleAndOffset_Colors[1] / 2 };
            for ( const auto& colorDesc : mDensityColors ) {
                scaleAndOffset[2] = static_cast<float>(colorDesc.first) / ApplicationDVR_common::numDensityBuckets;
                scaleAndOffset[2] -= scaleAndOffset[0] * 0.5f;
                shader.setVec4( "u_scaleOffset", scaleAndOffset );
                glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );
            }
            mpDensityColorDotTex2d->unbindFromTexUnit();
        }
    #if 1
        glDisable(GL_BLEND);
    #endif

        shader.use( false );
        glBindVertexArray( 0 );

        glCheckError();



        //glfwWaitEventsTimeout( 0.032f ); // wait time is in seconds
        glfwSwapBuffers( pWindow );

        const auto frameEndTime = std::chrono::system_clock::now();
        frameDelta = static_cast<float>(std::chrono::duration_cast<std::chrono::duration<double>>(frameEndTime - frameStartTime).count());
        frameDelta = linAlg::minimum( frameDelta, 0.032f );

        mouseWheelOffset = 0.0f;

        prevMouseX = currMouseX;
        prevMouseY = currMouseY;

        prevLeftMouseButtonPressed = leftMouseButtonPressed;

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

             mInterpolatedDataCPU[idx * 4 + 0] = static_cast<uint8_t>( mixRGB[0] * 255.0f );
             mInterpolatedDataCPU[idx * 4 + 1] = static_cast<uint8_t>( mixRGB[1] * 255.0f );
             mInterpolatedDataCPU[idx * 4 + 2] = static_cast<uint8_t>( mixRGB[2] * 255.0f );

             mInterpolatedDataCPU[idx * 4 + 3] = mTransparencyPaintHeightsCPU[idx];
         }
         
         currLeft = currRight;
    }

    //printf( "같같 ----- 같같\n" );
    mpDensityColorsTex2d->uploadData( mInterpolatedDataCPU.data(), GL_RGBA, GL_UNSIGNED_BYTE, 0 );

    mSharedMem.put( "TFcolorsAndAlpha", mInterpolatedDataCPU.data(), static_cast<uint32_t>( mInterpolatedDataCPU.size() ) );

}

void ApplicationTransferFunction::densityHistogramToTex2d() {
    std::vector<uint8_t> densityHistogramCPU;
    const uint32_t dim_x = mpDensityHistogramTex2d->desc().texDim[0];
    const uint32_t dim_y = mpDensityHistogramTex2d->desc().texDim[1];
    densityHistogramCPU.resize( dim_x * dim_y );
    std::fill( densityHistogramCPU.begin(), densityHistogramCPU.end(), 255 ); // set background to "white"

    uint32_t maxHistoVal = 1;
    for( uint32_t x = minRelevantDensity; x < mNumHistogramBuckets; x++ ) {
        maxHistoVal = linAlg::maximum( maxHistoVal, mHistogramBuckets[x] );
    }
    const float fRecipMaxHistoVal = 1.0f / static_cast<float>( maxHistoVal );

    for( uint32_t x = 0; x < dim_x; x++ ) {
        const auto histoVal = mHistogramBuckets[x];
        const float relativeHistoH = histoVal * fRecipMaxHistoVal;
        for ( uint32_t y = 0; y < static_cast<uint32_t>(relativeHistoH * dim_y); y++ ) {
            if ( y >= dim_y ) { break; }
            densityHistogramCPU[ y * dim_x + x ] = 120; // set color of current bar to slight gray
        }
    }

    mpDensityHistogramTex2d->uploadData( densityHistogramCPU.data(), GL_RED, GL_UNSIGNED_BYTE, 0 );
}

void ApplicationTransferFunction::densityTransparenciesToTex2d() {
    std::vector<uint8_t> densityTransparenciesCPU;
    const uint32_t dim_x = mpDensityTransparenciesTex2d->desc().texDim[0];
    const uint32_t dim_y = mpDensityTransparenciesTex2d->desc().texDim[1];
    densityTransparenciesCPU.resize( dim_x * dim_y * mpDensityTransparenciesTex2d->desc().numChannels ); // GL_RG texture
    std::fill( densityTransparenciesCPU.begin(), densityTransparenciesCPU.end(), 0 );

    constexpr int32_t kernelSize = 3;
    constexpr float fRecipKernelSize = 1.0f / static_cast<float>(kernelSize);

    //constexpr std::array< uint8_t, kernelSize + 1> kernelColorProile{ 255, 100, 150 }; // change of colors away from set transparency
    constexpr std::array< uint8_t, kernelSize + 1> kernelColorProile{ 0, 80, 170, 230 }; // change of colors away from set transparency
    constexpr std::array< uint8_t, kernelSize + 1> kernelAlphaProile{ 200, 64, 16, 4 }; // change of display transpareny for anti-aliasing of line (just for display

    for( uint32_t x = 0; x < dim_x; x++ ) {
        const auto transparencyVal = mTransparencyPaintHeightsCPU[x];
        //const float relativeHistoH = transparencyVal * fRecipMaxHistoVal;
        //for ( uint32_t y = 0; y < static_cast<uint32_t>(relativeHistoH * dim_y); y++ ) {
        //    if ( y >= dim_y ) { break; }
        //    densityTransparenciesCPU[ y * dim_x + x ] = 120;
        //}


        int32_t centerY = static_cast<int32_t>( static_cast<float>( transparencyVal ) * (1.0f/255.0f) * dim_y );
        for (  int32_t y = linAlg::maximum( 0, centerY - kernelSize ); y < linAlg::minimum<int32_t>( centerY + kernelSize, dim_y - 1 ); y++ ) {
            uint32_t addr = ( y * dim_x + x ) * 2;

            // addr + 0
            // addr + 1

            const int32_t absDistToCenter = abs(centerY - y);
            //const float fAbsDistToCenter01 = absDistToCenter * fRecipKernelSize;

            densityTransparenciesCPU[ addr + 0 ] = kernelColorProile[absDistToCenter];
            densityTransparenciesCPU[ addr + 1 ] = kernelAlphaProile[absDistToCenter];
        }
    }

    mpDensityTransparenciesTex2d->uploadData( densityTransparenciesCPU.data(), GL_RG, GL_UNSIGNED_BYTE, 0 );
}
