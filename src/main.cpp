#include "applicationDVR.h"
#include "applicationCreateVol.h"

#include "gfxAPI/contextOpenGL.h"

#include "argparse/argparse.h"

#include <stdlib.h>
#include <memory>

#include <thread>
#include <process.h>
#include <namedpipeapi.h>

#include <assert.h>

//#include <Windows.h>


using namespace GfxAPI;

static void threadFunc( void* pData ) {
    iApplication* pApp = reinterpret_cast<iApplication*>(pData);
    pApp->run();
}

int main( int argc, const char* argv[] )
{
#if 1
    for ( int32_t i = 0; i < argc; i++ ) {
        printf( "arg %d: %s\n", i, argv[ i ] );
    }
#endif

    argparse::ArgumentParser argParser{ argv[0], "Argument Parser"};

    argParser.add_argument()
        .names( { "-x", "--resx" } )
        .description( "screen resolution x" )
        .required( false );

    argParser.add_argument()
        .names( { "-y", "--resy" } )
        .description( "screen resolution y" )
        .required( false );

    argParser.add_argument()
        .names( { "--onlyProcess" } )
        .description( "only perform processing and close immediately" )
        .required( false );
    
    argParser.add_argument()
        .names( { "-t", "--transferFunction" } )
        .description( "transfer function mode" )
        .required( false );
    //Status_t cmdLineGrabRet = ApplicationProjectProxy::grabCmdLineArgs( argParser );    
    
    argParser.enable_help();

    const auto err = argParser.parse( argc, argv );
    if ( err ) {
        printf( "%s\n", err.what().c_str() );
        return EXIT_FAILURE;
    }

    if ( argParser.exists( "help" ) ) {
        argParser.print_help();
        return EXIT_SUCCESS;
    }

    int32_t resx = 1800;
    int32_t resy = resx;
    if ( argParser.exists( "x" ) ) { resx = argParser.get< int32_t >( "x" ); }
    if ( argParser.exists( "y" ) ) { resy = argParser.get< int32_t >( "y" ); }

    bool onlyProcess = false; 
    if ( argParser.exists( "onlyProcess" ) ) { onlyProcess = true; /* argParser.get< bool >( "onlyProcess" ); */ }

    
    std::thread* pOgl2_thread = nullptr;
    if ( !argParser.exists( "transferFunction" ) ) {
        //ContextOpenGL contextOpenGL2;

        //if (!onlyProcess) {
        //    ContextOpenGL::Settings_t settings;
        //    settings.windowTitle = "DVR Project";
        //    settings.windowW = resx/2;
        //    settings.windowH = resy/2;

        //    // mac os support stops at 4.1
        //    settings.glMajor = 3;
        //    settings.glMinor = 3;
        //    auto contextOpenGLStatus = contextOpenGL2.init( settings );
        //    assert( contextOpenGLStatus == Status_t::OK() );
        //}

        ////glfwMakeContextCurrent( reinterpret_cast<GLFWwindow*>(mpWindow) );
        ////std::shared_ptr< iApplication > pVolCreateApp{ new ApplicationCreateVol( contextOpenGL2, linAlg::i32vec3_t{512,64,128}, "./data/dummyvol.dat" ) };
        //std::shared_ptr< iApplication > pApp2{ new ApplicationDVR( contextOpenGL2 ) };

        //pOgl2_thread = new std::thread( threadFunc );
        //pOgl2_thread = new std::thread( [&]() {pApp2->run(); } );

        //_beginthread( threadFunc,
        //    1024,
        //    pApp2.get() );

        // maybe find a nice platform abstraction layer
        // maybe find exe of current exe...

    #if 0
        pOgl2_thread = new std::thread( [&]() {
                system( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe --transferFunction -x 800 -y 600)" ); // seems to block... perfect because the "outer" thread just continues to run..
                //execl( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe)", "--transferFunction", "-x", "400", "-y", "300" );
                //spawnl( P_NOWAIT, R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe)", "--transferFunction" );
            } );
        //pOgl2_thread->join();
        //system( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe --transferFunction)" );
        //execl( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe)", "--transferFunction");
        //spawnl( P_NOWAIT, R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe)", "--transferFunction" );
    #endif    
        

        //return EXIT_SUCCESS;
    }
    
    

    ContextOpenGL contextOpenGL;

    if (!onlyProcess) {
        ContextOpenGL::Settings_t settings;
        settings.windowTitle = "DVR Project";
        settings.windowW = resx;
        settings.windowH = resy;

        // mac os support stops at 4.1
        settings.glMajor = 3;
        settings.glMinor = 3;
        auto contextOpenGLStatus = contextOpenGL.init( settings );
        assert( contextOpenGLStatus == Status_t::OK() );
    }

    std::shared_ptr< iApplication > pVolCreateApp{ new ApplicationCreateVol( contextOpenGL, linAlg::i32vec3_t{512,64,128}, "./data/dummyvol.dat" ) };
    std::shared_ptr< iApplication > pApp{ new ApplicationDVR( contextOpenGL ) };

    pApp->run();

    return EXIT_SUCCESS;
}
