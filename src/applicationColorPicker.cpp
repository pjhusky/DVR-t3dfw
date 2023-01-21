#include "ApplicationColorPicker.h"

#include "math/linAlg.h"
#include "statusType.h"

#include "gfxUtils.h"
#include "gfxAPI/contextOpenGL.h"
#include "gfxAPI/shader.h"
#include "gfxAPI/texture.h"
#include "gfxAPI/checkErrorGL.h"

//#include "external/simdb/simdb.hpp"

#include <fstream>
#include <string>
#include <chrono>
#include <thread>
//

#define _USE_MATH_DEFINES
#include <math.h>


#include <GLFW/glfw3.h>

#include <cassert>


namespace {
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

    static void processInput( GLFWwindow *const pWindow ) {
        if ( glfwGetKey( pWindow, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) { glfwSetWindowShouldClose( pWindow, true ); }

        //float camSpeed = camSpeedFact;
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_RIGHT_SHIFT ) ) { camSpeed *= 0.1f; }

        //constexpr float flattenedCamSpeedFactor = 0.125f;
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_A ) ) { modelViewEFMatrix[ 0 ][ 3 ] += flattenedCamSpeedFactor * camSpeed * frameDelta; }
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_D ) ) { modelViewEFMatrix[ 0 ][ 3 ] -= flattenedCamSpeedFactor * camSpeed * frameDelta; }

        //if ( pressedOrRepeat( pWindow, GLFW_KEY_UP   ) ) { elevationRadians -= camSpeed * frameDelta; proxyRenderTargetNeedsUpdate = true; }
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_DOWN ) ) { elevationRadians += camSpeed * frameDelta; proxyRenderTargetNeedsUpdate = true; }

        //if ( pressedOrRepeat( pWindow, GLFW_KEY_LEFT  ) ) { azimuthRadians -= camSpeed * frameDelta; proxyRenderTargetNeedsUpdate = true; }
        //if ( pressedOrRepeat( pWindow, GLFW_KEY_RIGHT ) ) { azimuthRadians += camSpeed * frameDelta; proxyRenderTargetNeedsUpdate = true; }
    }

    static int32_t renderTargetDivFactor = 2;
    static bool forceScreenResize = false;
    static bool grabRenderedProxy = false;

    void keyboardCallback( GLFWwindow *const pWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods ) {
        //if ( key == GLFW_KEY_E && action == GLFW_RELEASE ) {
        //    renderModeEF = static_cast< eRenderMode >( 1 - static_cast< int32_t >( renderModeEF ) );
        //}
    }

#define STRIP_INCLUDES_FROM_INL 1
#include "applicationInterface/applicationGfxHelpers.inl"


    static int32_t toRenderTargetSize( const int32_t size ) {
        return size / renderTargetDivFactor;
    }

    static std::vector< uint32_t > fbos{ 0 };
    static GfxAPI::Texture colorRenderTargetTex;

    static void framebufferSizeCallback( GLFWwindow* pWindow, int width, int height ) {
        //glViewport( 0, 0, width, height );
        printf( "new window size %d x %d\n", width, height );

        glCheckError();

        //destroyRenderTargetTextures( fbos, colorRenderTargetTex, normalRenderTargetTex, silhouetteRenderTargetTex, depthRenderTargetTex );
        //createRenderTargetTextures( renderTargetW, renderTargetH, fbos, colorRenderTargetTex, normalRenderTargetTex, silhouetteRenderTargetTex, depthRenderTargetTex );

        glCheckError();

        //proxyRenderTargetNeedsUpdate = true;
    } 

    static void handleScreenResize( 
        GLFWwindow *const pWindow, 
        int32_t& prevFbWidth,
        int32_t& prevFbHeight,
        int32_t& fbWidth,
        int32_t& fbHeight ) {

        glfwGetFramebufferSize( pWindow, &fbWidth, &fbHeight);

        if ( prevFbWidth != fbWidth || prevFbHeight != fbHeight || forceScreenResize ) {
            
            printf( "polled new window size %d x %d\n", fbWidth, fbHeight );

            prevFbWidth = fbWidth;
            prevFbHeight = fbHeight;
            forceScreenResize = false;
        }
    }

} // namespace


ApplicationColorPicker::ApplicationColorPicker(
    const GfxAPI::ContextOpenGL& contextOpenGL ) 
    : mContextOpenGL( contextOpenGL ) {

    printf( "ApplicationColorPicker ctor begin\n" );

    if (!gladLoadGLLoader( reinterpret_cast<GLADloadproc>(glfwGetProcAddress) )) {
        //return Status_t::ERROR( "Failed to initialize GLAD" );
    }

    mScreenFill_VAO = gfxUtils::createScreenQuadGfxBuffers();

    mPrevFbWidth  = -1;
    mPrevFbHeight = -1;

    //std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > shaderDesc{
    //    std::make_pair( "./shaders/displayColors.vert.glsl", GfxAPI::Shader::eShaderStage::VS ),
    //    std::make_pair( "./shaders/displayColors.frag.glsl", GfxAPI::Shader::eShaderStage::FS ),
    //};
    //gfxUtils::createShader( *mpShader, shaderDesc );

    //FMProjectGUI::InitGui( contextOpenGL );
    printf( "ApplicationColorPicker ctor done\n" );
}



Status_t ApplicationColorPicker::drawGfx() {
    auto& shader = *mpShader;
    GLFWwindow* const pWindow = reinterpret_cast<GLFWwindow* const>(mContextOpenGL.window());

    glfwPollEvents();
    processInput( pWindow );

    GfxAPI::Shader& renderShader = shader;
    

    glCheckError();

    handleScreenResize(
        reinterpret_cast<GLFWwindow*>(mContextOpenGL.window()),
        mPrevFbWidth,
        mPrevFbHeight,
        mFbWidth,
        mFbHeight );

    if (mPrevFbWidth != mFbWidth || mPrevFbHeight != mFbHeight) {

        glfwGetFramebufferSize( reinterpret_cast<GLFWwindow*>(mContextOpenGL.window()), &mFbWidth, &mFbHeight );

        mPrevFbWidth = mFbWidth;
        mPrevFbHeight = mFbHeight;
    }



    glCheckError();


    #if 0
    glBindVertexArray( stlModel_VAO );
    renderShader.use( true );

    Texture::unbindAllTextures();



    //gfxUtils::createModelViewMatrixForModel(stlModel, azimuthRadians, elevationRadians, distFactor, modelViewMatrix);
    //linAlg::mat4_t mvpMatrix;
    linAlg::multMatrix( mvpMatrix, projMatrix, modelViewMatrix );
    auto retSetUniform = renderShader.setMat4( "u_mvpMat", mvpMatrix );
    std::vector<bool> displayMeshes;
    displayMeshes.push_back( true );

    if (displayMeshes[0]) {
        glDrawElements( GL_TRIANGLES, mStlModels[0].numStlIndices(), GL_UNSIGNED_INT, 0 );
    }

    //glBindVertexArray( 0 );
    #endif

    return Status_t::OK();
}

Status_t ApplicationColorPicker::run() {
#if 1

#if 0
    nanogui::init(); // seems to just init GLFW, which we've already done...

    /* scoped variables */ {
        bool use_gl_4_2 = true;// Set to true to create an OpenGL 4.1 context.
        //nanogui::ref<nanogui::Screen> screen;
        nanogui::ref<NanoApp> screen;

        if (use_gl_4_2) {
            // NanoGUI presents many options for you to utilize at your discretion.
            // See include/nanogui/screen.h for what all of these represent.
            //screen = new nanogui::Screen( nanogui::Vector2i( 500, 700 ), "NanoGUI test [GL 4.1]",
            screen = new NanoApp( nanogui::Vector2i( 500, 300 ), "NanoGUI test [GL 4.1]",
                /* resizable */ true, /* fullscreen */ false,
                /* depth_buffer */ true, /* stencil_buffer */ true,
                /* float_buffer */ false, /* gl_major */ 4,
                /* gl_minor */ 2 );
        }
        else {
            glCheckError();
            //screen = new nanogui::Screen( nanogui::Vector2i( 500, 700 ), "NanoGUI test" );
            screen = new NanoApp( nanogui::Vector2i( 500, 300 ), "NanoGUI test" );
            //screen = new nanogui::Screen();
            //screen = &aScreen;
            //screen->initialize( pWindow, false );
        }

        bool enabled = true;
        nanogui::FormHelper* gui = new nanogui::FormHelper( screen );
        nanogui::ref<nanogui::Window> window = gui->add_window( nanogui::Vector2i( 10, 10 ), "Form helper example" );
        gui->add_group( "Basic types" );
        gui->add_variable( "bool", bvar );
        gui->add_variable( "string", strval );
        gui->add_variable( "placeholder", strval2 )->set_placeholder( "placeholder" );

        gui->add_group( "Validating fields" );
        gui->add_variable( "int", ivar )->set_spinnable( true );
        gui->add_variable( "float", fvar );
        gui->add_variable( "double", dvar )->set_spinnable( true );

        gui->add_group( "Complex types" );
        gui->add_variable( "Enumeration", enumval, enabled )
            ->set_items( { "Item 1", "Item 2", "Item 3" } );
        gui->add_variable( "Color", colval );

        gui->add_group( "Other widgets" );
        gui->add_button( "A button", []() { std::cout << "Button pressed." << std::endl; } );

        screen->set_visible( true );
        screen->perform_layout();
        window->center();

        nanogui::mainloop( -1 );
        delete gui;
    }

    nanogui::shutdown();
    //return Status_t::OK();
#endif

    // handle window resize
    int32_t mFbWidth, mFbHeight;
    glfwGetFramebufferSize( reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ), &mFbWidth, &mFbHeight);
    //printf( "glfwGetFramebufferSize: %d x %d\n", mFbWidth, mFbHeight );

    
    
    //uint32_t stlModel_VAO = gfxUtils::createMeshGfxBuffers(gfxCoords.size() / 3, gfxCoords, gfxNormals.size() / 3, gfxNormals, gfxTriangleVertexIndices.size(), gfxTriangleVertexIndices);
    //uint32_t stlModel_VAO = gfxUtils::createMeshGfxBuffers(
    //    mStlModels[0].gfxCoords().size() / 3, 
    //    mStlModels[0].gfxCoords(), 
    //    mStlModels[0].gfxNormals().size() / 3, 
    //    mStlModels[0].gfxNormals(), 
    //    mStlModels[0].gfxTriangleVertexIndices().size(), 
    //    mStlModels[0].gfxTriangleVertexIndices());


    // load shaders
    printf( "creating mesh shader\n" ); fflush( stdout );
    //Shader shader;
    //std::vector< std::pair< gfxUtils::path_t, GfxAPI::Shader::eShaderStage > > shaderDesc{ 
    //    std::make_pair( "./shaders/stlModel.vert.glsl", GfxAPI::Shader::eShaderStage::VS ),
    //    std::make_pair( "./shaders/stlModel.frag.glsl", GfxAPI::Shader::eShaderStage::FS ),
    //};
    //gfxUtils::createShader( shader, shaderDesc );

    
    GLFWwindow *const pWindow = reinterpret_cast< GLFWwindow *const >( mContextOpenGL.window() );

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ); // not necessary
    glDisable( GL_CULL_FACE );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );



    //int32_t mPrevFbWidth = -1;
    //int32_t mPrevFbHeight = -1;
    //linAlg::mat4_t projMatrix;

    constexpr int32_t numSrcChannels = 3;
    std::shared_ptr< uint8_t > pGrabbedFramebuffer = std::shared_ptr< uint8_t >( new uint8_t[ mFbWidth * mFbHeight * numSrcChannels ], std::default_delete< uint8_t[] >() );

    glEnable( GL_DEPTH_TEST );


    glfwSetKeyCallback( pWindow, keyboardCallback );
    glfwSetFramebufferSizeCallback( pWindow, framebufferSizeCallback );

    glCheckError();

    uint64_t frameNum = 0;
    double dCurrMouseX, dCurrMouseY;

    while( !glfwWindowShouldClose( pWindow ) ) {

        //printf("heyho!\n");
        glfwPollEvents();
        processInput( pWindow );
        glfwGetCursorPos( pWindow, &dCurrMouseX, &dCurrMouseY );


        const auto frameStartTime = std::chrono::system_clock::now();
        
        glViewport( 0, 0, mFbWidth, mFbHeight ); // set to render-target size

        handleScreenResize(
            reinterpret_cast< GLFWwindow* >( mContextOpenGL.window() ),
            mPrevFbWidth,
            mPrevFbHeight,
            mFbWidth,
            mFbHeight );

        { // clear screen
            constexpr float clearColorValue[]{ 0.5f, 0.0f, 0.0f, 0.7f };
            glClearBufferfv( GL_COLOR, 0, clearColorValue );
            constexpr float clearDepthValue = 1.0f;
            glClearBufferfv( GL_DEPTH, 0, &clearDepthValue );
        }

        for ( int32_t i = 0; i < 10000; i++ ) {
            if ( i * i % 234435 == 32 ) { printf( "blah\n" ); }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        //drawGfx();

        glCheckError();

        //glfwWaitEventsTimeout( 0.032f ); // wait time is in seconds
        glfwSwapBuffers( pWindow );

        const auto frameEndTime = std::chrono::system_clock::now();
        frameDelta = std::chrono::duration_cast<std::chrono::duration<double>>( frameEndTime - frameStartTime ).count();
        frameDelta = linAlg::minimum( frameDelta, 0.032f );

        frameNum++;
    }

    //destroyRenderTargetTextures( fbos, colorRenderTargetTex, normalRenderTargetTex, silhouetteRenderTargetTex, depthRenderTargetTex );

    #if 0
        std::stringstream commentStream;
        commentStream << "# grabbed framebuffer: " << 1 << "\n";
        constexpr bool ppmBinary = false;
        netBpmIO::writePpm( "grabbedFramebuffer.ppm", pGrabbedFramebuffer.get(), numSrcChannels, mFbWidth, mFbHeight, ppmBinary, commentStream.str() );
    #endif
#endif 
    return Status_t::OK();
}
