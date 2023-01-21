#include "applicationDVR.h"
#include "applicationCreateVol.h"

#include "gfxAPI/contextOpenGL.h"

#include "argparse/argparse.h"

#include "external/tiny-process-library/process.hpp"
#include "external/simdb/simdb.hpp"
#include <tchar.h>
#include <filesystem>

#include <stdlib.h>
#include <memory>

#include <thread>
#include <process.h>
#include <namedpipeapi.h>

#include <assert.h>

#include <Windows.h>

static void threadFunc( void* pData ) {
    iApplication* pApp = reinterpret_cast<iApplication*>(pData);
    pApp->run();
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


    const uint32_t smBlockSizeInBytes = 1024u;
    const uint32_t smNumBlocks = 4096u;
    simdb sharedMem( "DVR_shared_memory", smBlockSizeInBytes, smNumBlocks );
    
    auto sharedMemFiles = simdb_listDBs();
    sharedMem.put( "lock free", "is the way to be" );

    const auto queriedSmVal = sharedMem.get( "lock free" );

    std::thread* pOgl2_thread = nullptr;
    if ( !argParser.exists( "transferFunction" ) ) {

    #if 0
        pOgl2_thread = new std::thread( [&]() {
                system( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe --transferFunction -x 800 -y 600)" ); // seems to block... perfect because the "outer" thread just continues to run..
                //execl( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe)", "--transferFunction", "-x", "400", "-y", "300" );
                //spawnl( P_NOWAIT, R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe)", "--transferFunction" );
            } );
        //pOgl2_thread->join();
    #endif    
        
    #if 1
        
        #if 0
            STARTUPINFO si;
            ::memset( &si, 0, sizeof( si ) );
            si.cb = sizeof( si );
            PROCESS_INFORMATION pi;
            std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
            cmdLine = cmdLine.append( _TEXT("  --transferFunction -x 800 -y 600") );
            auto processHandle = ::CreateProcess( NULL, const_cast<TCHAR*>( cmdLine.c_str() ), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
        #elif 0
            pOgl2_thread = new std::thread( [&]() {
                STARTUPINFO si;
                ::memset( &si, 0, sizeof( si ) );
                si.cb = sizeof( si );
                PROCESS_INFORMATION pi;
                std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
                cmdLine = cmdLine.append( _TEXT("  --transferFunction -x 800 -y 600") );
                auto processHandle = ::CreateProcess( NULL, const_cast<TCHAR*>( cmdLine.c_str() ), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
                // Wait until child process exits.
                WaitForSingleObject( pi.hProcess, INFINITE );
                // Close process and thread handles. 
                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
            } );
        #elif 0
            pOgl2_thread = new std::thread( [&]() {
                std::string args{ argv[0] };
                //system( R"(.\bin\Win64\Release\T3DFW_DVR_Project.exe --transferFunction -x 800 -y 600)" );
                const auto processStartCmd = (args + " --transferFunction -x 800 -y 600)");
                printf( "processStartCmd '%s'\n", processStartCmd.c_str() );
                system( processStartCmd.c_str() );
            } );
        #elif 0
            std::basic_string<TCHAR> cmdLine = ::GetCommandLine();
            cmdLine = cmdLine.append( _TEXT( "  --transferFunction -x 800 -y 600" ) );

            auto transferFuncApp = TinyProcessLib::Process( cmdLine );
        #elif 1
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
        #endif
        
    #endif
    }
    
    

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

    std::shared_ptr< iApplication > pVolCreateApp{ new ApplicationCreateVol( contextOpenGL, linAlg::i32vec3_t{512,64,128}, "./data/dummyvol.dat" ) };
    std::shared_ptr< iApplication > pApp{ new ApplicationDVR( contextOpenGL ) };

    pApp->run();

    return EXIT_SUCCESS;
}
