#include "gfxAPI/contextOpenGL.h"

#include "applicationDVR.h"
#include "applicationCreateVol.h"
#include "applicationTransferFunction.h"
#include "applicationColorPicker.h"


#include "argparse/argparse.h"

#include "stringUtils.h"

//#include "external/tiny-process-library/process.hpp"
//#include "external/simdb/simdb.hpp"
#include "sharedMemIPC.h"

#include <string>
#include <tchar.h>
#include <filesystem>

#include <stdlib.h>
#include <memory>

#include <thread>
#include <process.h>
//#include <namedpipeapi.h>

#include <assert.h>

#include <Windows.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"


namespace {
    static void threadFunc( void* pData ) {
        iApplication* pApp = reinterpret_cast<iApplication*>(pData);
        pApp->run();
    }

    static const std::vector<TinyProcessLib::Process::string_type>& CommandLinePath_TransferFunction( GfxAPI::ContextOpenGL& contextOpenGl, const char *const argv ) {
        static std::vector<TinyProcessLib::Process::string_type> cmdLinePath{};

        if (cmdLinePath.empty()) {

            float sx, sy;
            glfwGetWindowContentScale( reinterpret_cast<GLFWwindow*>(contextOpenGl.window()), &sx, &sy );
            int32_t desiredWindowW = static_cast<int32_t>( 1024.0f * sx );
            int32_t desiredWindowH = static_cast<int32_t>(  512.0f * sy );

            auto strWindowW = stringUtils::ToString( desiredWindowW );
            auto strWindowH = stringUtils::ToString( desiredWindowH );            

            std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
            auto argv0Wide = std::filesystem::_Convert_Source_to_wide( argv );
            //const std::vector<TinyProcessLib::Process::string_type> arguments{
            cmdLinePath = std::vector<TinyProcessLib::Process::string_type>{
                //cmdLine.c_str(),
                argv0Wide,
                TinyProcessLib::Process::string_type{ _TEXT( "--transferFunction" ) },
                _T( "-x" ),
                strWindowW.c_str(), //_T( "1200" ),
                _T( "-y" ),
                strWindowH.c_str()  //_T( "1200" ) 
            };
        }

        return cmdLinePath;
    }

    static const std::vector<TinyProcessLib::Process::string_type>& CommandLinePath_ColorPicker( GfxAPI::ContextOpenGL& contextOpenGl, const char* const argv ) {
        static std::vector<TinyProcessLib::Process::string_type> cmdLinePath{};

        if (cmdLinePath.empty()) {

            float sx, sy;
            glfwGetWindowContentScale( reinterpret_cast<GLFWwindow*>(contextOpenGl.window()), &sx, &sy );
            int32_t desiredWindowW = static_cast<int32_t>( 600.0f * sx );
            int32_t desiredWindowH = static_cast<int32_t>( 600.0f * sy );

            auto strWindowW = stringUtils::ToString( desiredWindowW );
            auto strWindowH = stringUtils::ToString( desiredWindowH );            

            std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
            auto argv0Wide = std::filesystem::_Convert_Source_to_wide( argv );
            cmdLinePath = std::vector<TinyProcessLib::Process::string_type>{
                //cmdLine.c_str(),
                argv0Wide,
                TinyProcessLib::Process::string_type{ _TEXT( "--colorPicker" ) },
                _T( "-x" ),
                strWindowW.c_str(), //_T( "600" ),
                _T( "-y" ),
                strWindowH.c_str()  //_T( "600" ) 
            };
        }

        return cmdLinePath;
    }

}

int main( int argc, const char* argv[] )
//int _tmain( int argc, TCHAR* argv[] )
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
    argParser.add_argument()
        .names( { "-c", "--colorPicker" } )
        .description( "color picker mode" )
        .required( false );

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


    //const uint32_t smBlockSizeInBytes = 1024u;
    //const uint32_t smNumBlocks = 4096u;
    //simdb sharedMem( "DVR_shared_memory", smBlockSizeInBytes, smNumBlocks );
    //
    //auto sharedMemFiles = simdb_listDBs();
    //sharedMem.put( "lock free", "is the way to be" );

    //const auto queriedSmVal = sharedMem.get( "lock free" );

    SharedMemIPC sharedMem( "DVR_shared_memory" );
    auto sharedMemFiles = SharedMemIPC::listSharedMemFiles();
    sharedMem.put( "lock free", "is the way to be" );
    const auto queriedSmVal = sharedMem.get( "lock free" );

#if 0
    if ( !argParser.exists( "transferFunction" ) ) {        
        std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
        auto argv0Wide = std::filesystem::_Convert_Source_to_wide( argv[0] );
        const std::vector<TinyProcessLib::Process::string_type> arguments{ 
            //cmdLine.c_str(),
            argv0Wide,
            TinyProcessLib::Process::string_type{ _TEXT( "--transferFunction" ) }, 
            _T( "-x" ), 
            _T( "800" ), 
            _T( "-y" ), 
            _T( "600" ) };

        std::filesystem::path cmdLinePath{ cmdLine };
        auto cmdPathNarrow = cmdLinePath.parent_path().string();
        auto cmdPathWide = std::filesystem::_Convert_Source_to_wide( cmdPathNarrow );
        auto transferFuncApp = TinyProcessLib::Process( arguments );
    }
#endif
    


    GfxAPI::ContextOpenGL contextOpenGL;

    if (!onlyProcess) {
        GfxAPI::ContextOpenGL::Settings_t settings;
        settings.windowTitle = "DVR Project";
        settings.windowW = resx;
        settings.windowH = resy;

        // mac os support stops at 4.1
        settings.glMajor = 3;
        settings.glMinor = 3;
        auto contextOpenGLStatus = contextOpenGL.init( settings );
        assert( contextOpenGLStatus == Status_t::OK() );
    }

    //std::shared_ptr< iApplication > pApp{ new ApplicationColorPicker( contextOpenGL ) };
    //pApp->run();
    //return 0;

#if 0
    const bool checkWatchdog = false;
    std::shared_ptr< iApplication > pApp{ new ApplicationTransferFunction( contextOpenGL, checkWatchdog ) };
    reinterpret_cast<ApplicationTransferFunction*>(pApp.get())->setCommandLinePath( CommandLinePath_ColorPicker( contextOpenGL, argv[0] ) );
    pApp->run();
    return 0;
#endif

    if (argParser.exists( "transferFunction" )) {
        printf( "main: spin up transfer-function app!\n" );
        std::shared_ptr< iApplication > pApp{ new ApplicationTransferFunction( contextOpenGL ) };
        reinterpret_cast<ApplicationTransferFunction*>(pApp.get())->setCommandLinePath( CommandLinePath_ColorPicker( contextOpenGL, argv[0] ) );
        pApp->run();
    } else if (argParser.exists( "colorPicker" )) {
        printf( "main: spin up colorPicker app!\n" );
        std::shared_ptr< iApplication > pApp{ new ApplicationColorPicker( contextOpenGL ) };
        pApp->run();
    } else {
        std::shared_ptr< iApplication > pVolCreateApp{ new ApplicationCreateVol( contextOpenGL, linAlg::i32vec3_t{512,64,128}, "./data/dummyvol.dat" ) };
        std::shared_ptr< iApplication > pApp{ new ApplicationDVR( contextOpenGL ) };
        reinterpret_cast<ApplicationDVR*>(pApp.get())->setCommandLinePath( CommandLinePath_TransferFunction( contextOpenGL, argv[0] ) );
        pApp->run();
    }

    return EXIT_SUCCESS;
}
