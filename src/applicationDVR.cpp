// TODO: 
// for the unit-cube raymarcher: handle cam-in-box case => Problem is glaub ich nur die near plane, die dann die backfaces beginnt wegzuklippen weshalb ich dann die ray-exit Schnittpunkte verlier (und somit den gesamten Strahl durch dieses Pixel)
// noisy offset um banding artifacts zu minimieren - m�gl.weise gepaart mit dem Ansatz zum Sobel-normal filtering

#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include <math.h>
#include <iostream>

#include "ApplicationDVR.h"
#include "fileLoaders/volumeData.h"
#include "fileLoaders/offLoader.h"

#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/checkErrorGL.h"

#include "math/linAlg.h"

#include "gfxUtils.h"

#include "arcBall/arcBallControls.h"

#include "GUI/DVR_GUI.h"

#include <memory>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#include "fileLoaders/stlModel.h" // used for the unit-cube

#include "gfxAPI/contextOpenGL.h"

#include <cassert>

using namespace ArcBall;

namespace {
    constexpr float angleDamping = 0.85f;
    constexpr float panDamping = 0.25f;
    constexpr float mouseSensitivity = 0.23f;
    constexpr float zoomSpeed = 1.5f;

    static DVR_GUI::eRayMarchAlgo rayMarchAlgo = DVR_GUI::eRayMarchAlgo::fullscreenBoxIsect;

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
    
    static void processInput( GLFWwindow *const pWindow ) {
        if ( glfwGetKey( pWindow, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) { glfwSetWindowShouldClose( pWindow, true ); }

        float camSpeed = camSpeedFact;
        if ( pressedOrRepeat( pWindow, GLFW_KEY_RIGHT_SHIFT ) ) { camSpeed *= 0.1f; }

        if (pressedOrRepeat( pWindow, GLFW_KEY_UP )) { targetPanDeltaVector[1] -= camSpeed * frameDelta; }
        if (pressedOrRepeat( pWindow, GLFW_KEY_DOWN )) { targetPanDeltaVector[1] += camSpeed * frameDelta; }

        if (pressedOrRepeat( pWindow, GLFW_KEY_LEFT )) { targetPanDeltaVector[0] += camSpeed * frameDelta; }
        if (pressedOrRepeat( pWindow, GLFW_KEY_RIGHT )) { targetPanDeltaVector[0] -= camSpeed * frameDelta; }
    }

    static void keyboardCallback( GLFWwindow *const pWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods ) {
        //if ( key == GLFW_KEY_E && action == GLFW_RELEASE ) {
        //}
    }

    static float mouseWheelOffset = 0.0f;
    static void mouseWheelCallback( GLFWwindow* window, double xoffset, double yoffset ) {
        mouseWheelOffset = yoffset;
    }
    

#define STRIP_INCLUDES_FROM_INL 1
#include "applicationInterface/applicationGfxHelpers.inl"

    static std::vector< uint32_t > fbos{ 0 };
    static Texture colorRenderTargetTex;
    static Texture silhouetteRenderTargetTex;
    static Texture normalRenderTargetTex;
    static Texture depthRenderTargetTex;

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
    const ContextOpenGL& contextOpenGL ) 
    : mContextOpenGL( contextOpenGL ) 
    , mDataFileUrl( "" )
    , mpData( nullptr )
    , mpDensityTex3d( nullptr )
    , mpNormalTex3d( nullptr ) {

    DVR_GUI::InitGui( contextOpenGL );
}

ApplicationDVR::~ApplicationDVR() {
    delete mpData;
    mpData = nullptr;
    delete mpDensityTex3d;
    mpDensityTex3d = nullptr;
    delete mpNormalTex3d;
    mpNormalTex3d = nullptr;
}

Status_t ApplicationDVR::load( const std::string& fileUrl )
{
    mDataFileUrl = fileUrl;

    delete mpData;
    mpData = new VolumeData;
    mpData->load( fileUrl.c_str() );

    const auto volDim = mpData->getDim();
    Texture::Desc_t volTexDesc{
        .texDim = linAlg::i32vec3_t{ volDim[0], volDim[1], volDim[2] },
        .numChannels = 1,
        .channelType = Texture::eChannelType::i16,
        .semantics = Texture::eSemantics::color,
        .isMipMapped = false,
    };
    delete mpDensityTex3d;

    mpDensityTex3d = new Texture;
    mpDensityTex3d->create( volTexDesc );
    const uint32_t mipLvl = 0;
    mpDensityTex3d->uploadData( mpData->getDensities().data(), GL_RED, GL_UNSIGNED_SHORT, mipLvl );
    mpDensityTex3d->setWrapModeForDimension( Texture::eBorderMode::clamp, 0 );
    mpDensityTex3d->setWrapModeForDimension( Texture::eBorderMode::clamp, 1 );
    mpDensityTex3d->setWrapModeForDimension( Texture::eBorderMode::clamp, 2 );

    return Status_t::OK();
}

Status_t ApplicationDVR::run() {
    ArcBallControls arcBallControl;
    
    // handle window resize
    int32_t fbWidth, fbHeight;
    glfwGetFramebufferSize( reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ), &fbWidth, &fbHeight);
    //printf( "glfwGetFramebufferSize: %d x %d\n", fbWidth, fbHeight );

    uint32_t screen_VAO = gfxUtils::createScreenQuadGfxBuffers();

    // load shaders
    printf( "creating volume shader\n" ); fflush( stdout );
    Shader volShader;
    std::vector< std::pair< gfxUtils::path_t, Shader::eShaderStage > > volShaderDesc{
        std::make_pair( "./src/shaders/tex3d-raycast.vert.glsl", Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/tex3d-raycast.frag.glsl", Shader::eShaderStage::FS ), // X-ray of x-y planes
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

    linAlg::mat4_t modelViewMatrix;

    int32_t prevFbWidth = -1;
    int32_t prevFbHeight = -1;
    linAlg::mat4_t projMatrix;

    constexpr int32_t numSrcChannels = 3;
    std::shared_ptr< uint8_t > pGrabbedFramebuffer = std::shared_ptr< uint8_t >( new uint8_t[ fbWidth * fbHeight * numSrcChannels ], std::default_delete< uint8_t[] >() );

    glEnable( GL_DEPTH_TEST );

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
    linAlg::mat3x4_t invTiltRotMat;


    StlModel stlModel;
    stlModel.load( "./src/data/blender-cube-ascii.STL" );
    //stlModel.load( "./data/blender-cube.STL" );

    uint32_t stlModel_VAO;
    stlModel_VAO = gfxUtils::createMeshGfxBuffers(
        stlModel.gfxCoords().size() / 3,
        stlModel.gfxCoords(),
        stlModel.gfxNormals().size() / 3,
        stlModel.gfxNormals(),
        stlModel.gfxTriangleVertexIndices().size(),
        stlModel.gfxTriangleVertexIndices() );
    Shader meshShader;
    std::vector< std::pair< gfxUtils::path_t, Shader::eShaderStage > > meshShaderDesc{
        std::make_pair( "./src/shaders/rayMarchUnitCube.vert.glsl", Shader::eShaderStage::VS ),
        std::make_pair( "./src/shaders/rayMarchUnitCube.frag.glsl", Shader::eShaderStage::FS ),
    };
    gfxUtils::createShader( meshShader, meshShaderDesc );
    meshShader.use( true );
    meshShader.setInt( "u_densityTex", 0 );
    meshShader.use( false );

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

        bool leftMouseButtonPressed = (glfwGetMouseButton( pWindow, GLFW_MOUSE_BUTTON_1 ) == GLFW_PRESS);
        bool middleMouseButtonPressed = (glfwGetMouseButton( pWindow, GLFW_MOUSE_BUTTON_3 ) == GLFW_PRESS);
        bool rightMouseButtonPressed = (glfwGetMouseButton( pWindow, GLFW_MOUSE_BUTTON_2 ) == GLFW_PRESS);

        if ((leftMouseButton_down && (targetMouse_dx * targetMouse_dx + targetMouse_dy * targetMouse_dy > 0.01)) || rightMouseButtonPressed || middleMouseButtonPressed) {
            glfwSetInputMode( pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
        }
        else {
            glfwSetInputMode( pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
        }

        if (rightMouseButtonPressed) {
            //printf( "RMB pressed!\n" );
            targetCamZoomDist += mouse_dy / (fbHeight * mouseSensitivity * 0.5f);
            targetCamTiltRadAngle -= mouse_dx / (fbWidth * mouseSensitivity * 0.5f);
        }
        if (middleMouseButtonPressed) {
            //printf( "MMB pressed!\n" );
            targetPanDeltaVector[0] += mouse_dx * frameDelta * mouseSensitivity;
            targetPanDeltaVector[1] -= mouse_dy * frameDelta * mouseSensitivity;
        }

        glViewport( 0, 0, fbWidth, fbHeight ); // set to render-target size
        { // clear screen
            constexpr float clearColorValue[]{ 0.0f, 0.5f, 0.0f, 0.0f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
        }

        glCheckError();

        if (leftMouseButton_down) {
            targetMouse_dx += mouse_dx;
            targetMouse_dy += mouse_dy;
        }

        linAlg::mat3_t rolledRefFrameMatT;
        {
            linAlg::mat3x4_t camRollMat;
            linAlg::loadRotationZMatrix( camRollMat, camTiltRadAngle );

            linAlg::mat3_t rolledRefFrameMat;
            linAlg::vec3_t refX;
            linAlg::cast( refX, camRollMat[0] );
            linAlg::vec3_t refY;
            linAlg::cast( refY, camRollMat[1] );
            linAlg::vec3_t refZ;
            linAlg::cast( refZ, camRollMat[2] );
            rolledRefFrameMat[0] = refX;
            rolledRefFrameMat[1] = refY;
            rolledRefFrameMat[2] = refZ;

            linAlg::transpose( rolledRefFrameMatT, rolledRefFrameMat );

            arcBallControl.setRefFrameMat( rolledRefFrameMatT );
        }

        arcBallControl.update( currMouseX, currMouseY, leftMouseButtonPressed, rightMouseButtonPressed, fbWidth, fbHeight );

        const linAlg::mat3x4_t& arcRotMat = arcBallControl.getRotationMatrix();
        linAlg::vec3_t volDimRatio{ 1.0f, 1.0f, 1.0f };

        {
            linAlg::mat3x4_t centerTranslationMatrix;

            linAlg::vec4_t boundingSphere;
            stlModel.getBoundingSphere( boundingSphere );
            linAlg::loadTranslationMatrix( centerTranslationMatrix, linAlg::vec3_t{ -boundingSphere[0], -boundingSphere[1], -boundingSphere[2] } );

            constexpr float modelScaleFactor = 1.0f;
            linAlg::mat3x4_t scaleMatrix;

            linAlg::loadScaleMatrix( scaleMatrix, linAlg::vec3_t{ modelScaleFactor, modelScaleFactor, modelScaleFactor } );
            
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

            linAlg::mat3x4_t viewTranslationMatrix;
            // calc required translation based on FOV and bounding-sphere size
            linAlg::loadTranslationMatrix( viewTranslationMatrix, linAlg::vec3_t{ 0.0f, 0.0f, -boundingSphere[3] * modelScaleFactor * camZoomDist } );

            linAlg::mat3x4_t modelMatrix3x4;
            linAlg::mat3x4_t tmpModelMatrixPre;

            linAlg::multMatrix( tmpModelMatrixPre, scaleMatrix, centerTranslationMatrix );

            linAlg::mat3x4_t viewRotMatrix;
            linAlg::multMatrix( viewRotMatrix, arcRotMat, tmpModelMatrixPre );

            linAlg::multMatrix( modelMatrix3x4, viewTranslationMatrix, viewRotMatrix );

            linAlg::vec3_t zAxis{ 0.0f, 0.0f, 1.0f };
            linAlg::loadRotationZMatrix( tiltRotMat, camTiltRadAngle );
            linAlg::mat3x4_t tmpModelMatrix3x4;
            linAlg::multMatrix( tmpModelMatrix3x4, tiltRotMat, modelMatrix3x4 );
            modelMatrix3x4 = tmpModelMatrix3x4;

            panVector[0] += targetPanDeltaVector[0];
            panVector[1] += targetPanDeltaVector[1];
            linAlg::vec3_t panVec3{ panVector[0], panVector[1], 0.0f };
            linAlg::mat3x4_t panMat;
            linAlg::loadTranslationMatrix( panMat, panVec3 );
            linAlg::mat3x4_t panMVMat;
            linAlg::multMatrix( panMVMat, panMat, modelMatrix3x4 );
            modelMatrix3x4 = panMVMat;

            linAlg::cast( modelViewMatrix, modelMatrix3x4 );
        }

        handleScreenResize(
            reinterpret_cast<GLFWwindow*>(mContextOpenGL.window()),
            projMatrix,
            prevFbWidth,
            prevFbHeight,
            fbWidth,
            fbHeight );

        glfwGetFramebufferSize( reinterpret_cast<GLFWwindow*>(mContextOpenGL.window()), &fbWidth, &fbHeight );

        if (prevFbWidth != fbWidth || prevFbHeight != fbHeight)
        {
            calculateProjectionMatrix( fbWidth, fbHeight, projMatrix );

            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
        }

        glCheckError();

        // TODO: draw screen-aligned quad to lauch one ray per pixel
        //       dual view on transformation: inverse model-view transform should place cam frame such that it sees the model as usual, but now model is in origin and axis aligned
        //       this should make data traversal easier...


    #if 1 // unit-cube STL file
        if ( rayMarchAlgo == DVR_GUI::eRayMarchAlgo::backfaceCubeRaster ) {
            glEnable( GL_CULL_FACE );
            glCullFace( GL_FRONT );

            glBindVertexArray( stlModel_VAO );
            meshShader.use( true );

            Texture::unbindAllTextures();

            linAlg::mat4_t mvpMatrix;
            linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );
            auto retSetUniform = meshShader.setMat4( "u_mvpMat", mvpMatrix );
            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->bindToTexUnit( 0 );
            }

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, modelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformation( invModelViewMatrix, &camPos_OS, 1 );
            meshShader.setVec4( "u_camPos_OS", camPos_OS );
            meshShader.setVec3( "u_volDimRatio", volDimRatio );

            glDrawElements( GL_TRIANGLES, stlModel.numStlIndices(), GL_UNSIGNED_INT, 0 );

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
            glBindVertexArray( screen_VAO );

            linAlg::mat4_t invModelViewMatrix;
            linAlg::inverse( invModelViewMatrix, modelViewMatrix );
            const linAlg::vec4_t camPos_ES{ 0.0f, 0.0f, 0.0f, 1.0f };
            linAlg::vec4_t camPos_OS = camPos_ES;
            linAlg::applyTransformation( invModelViewMatrix, &camPos_OS, 1 );
            volShader.setVec4( "u_camPos_OS", camPos_OS );
            volShader.setVec3( "u_volDimRatio", volDimRatio );

            linAlg::mat4_t mvpMatrix;
            linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );

            linAlg::mat4_t invModelViewProjectionMatrix;
            linAlg::inverse( invModelViewProjectionMatrix, mvpMatrix );

            constexpr float fpDist_NDC = +1.0f;
            std::array< linAlg::vec4_t, 4 > cornersFarPlane_NDC{
                linAlg::vec4_t{ -1.0f, +1.0f, fpDist_NDC, 1.0f }, // L top
                linAlg::vec4_t{ -1.0f, -1.0f, fpDist_NDC, 1.0f }, // L bottom
                linAlg::vec4_t{ +1.0f, -1.0f, fpDist_NDC, 1.0f }, // R bottom
                linAlg::vec4_t{ +1.0f, +1.0f, fpDist_NDC, 1.0f }, // R top
            };
            
            // transform points to Object Space of Volume Data
            linAlg::applyTransformation( invModelViewProjectionMatrix, cornersFarPlane_NDC.data(), cornersFarPlane_NDC.size() );

            volShader.setVec4Array( "u_fpDist_OS", cornersFarPlane_NDC.data(), 4 );

            glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr );

            if (mpDensityTex3d != nullptr) {
                mpDensityTex3d->unbindFromTexUnit();
            }
            volShader.use( false );
        }
    #endif

        glCheckError();

        {
            static bool loadFileTrigger = false;
            static bool resetTrafos = false;
            int rayMarchAlgoIdx = static_cast<int>(rayMarchAlgo);
            int* pRayMarchAlgoIdx = &rayMarchAlgoIdx;
            DVR_GUI::GuiUserData_t guiUserData{
                .volumeDataUrl = mDataFileUrl,
                .pRayMarchAlgoIdx = pRayMarchAlgoIdx,
                .loadFileTrigger = loadFileTrigger,
                .resetTrafos = resetTrafos,
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
            

        }

        //glfwWaitEventsTimeout( 0.032f ); // wait time is in seconds
        glfwSwapBuffers( pWindow );

        const auto frameEndTime = std::chrono::system_clock::now();
        frameDelta = std::chrono::duration_cast<std::chrono::duration<double>>(frameEndTime - frameStartTime).count();
        frameDelta = linAlg::minimum( frameDelta, 0.032f );

        targetCamZoomDist += mouseWheelOffset * zoomSpeed;
        targetCamZoomDist = linAlg::clamp( targetCamZoomDist, 0.1f, 1000.0f );
        camZoomDist = targetCamZoomDist * (1.0f - angleDamping) + camZoomDist * angleDamping;
        mouseWheelOffset = 0.0f;

        camTiltRadAngle = targetCamTiltRadAngle * (1.0f - angleDamping) + camTiltRadAngle * angleDamping;

        linAlg::scale( targetPanDeltaVector, (1.0f - panDamping) );

        prevMouseX = currMouseX;
        prevMouseY = currMouseY;

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
    targetCamTiltRadAngle = 0.0f;
    targetPanDeltaVector = linAlg::vec3_t{ 0.0f, 0.0f, 0.0f };
}
