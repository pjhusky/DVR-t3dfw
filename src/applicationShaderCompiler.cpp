#include "applicationShaderCompiler.h"

#include "fileUtils.h"
#include "utf8Utils.h" // solve C++17 UTF deprecation

#include <stdio.h>

#include <iostream>
#include <tchar.h>


ApplicationShaderCompiler::ApplicationShaderCompiler( const std::string& shaderDefines /*const std::filesystem::path& baseShaderDir*/ )
    : mpProcess( nullptr )
    , mShaderDefines( shaderDefines ) {
}

Status_t ApplicationShaderCompiler::run()
{
    //std::vector< std::string > shaderFilenames;
    //const std::regex glslRegex{ R"([\s\S]*.glsl$)" };
    //fileUtils::lukeFileWalker( 0, baseShaderDir, shaderFilenames, glslRegex );

    printf( "appSC: mpProcess is nullptr? %s\n", ( mpProcess == nullptr ) ? "yes" : "no" );


    
    std::vector< std::string > shaderFilenames;
    const std::regex glslRegex{ R"([\s\S]*[^h].glsl$)" }; // skip shader header files *.h.glsl
    const std::filesystem::path baseShaderDir{ "./src/shaders/" };
    fileUtils::lukeFileWalker( 0, baseShaderDir, shaderFilenames, glslRegex );

    const auto argv0Wide =
    #ifndef UNICODE 
        std::filesystem::absolute( std::filesystem::path( R"(.\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe)" ) ).generic_string();
    #else
        std::filesystem::absolute( std::filesystem::path( R"(.\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe)" ) ).native(); //+
    #endif

    //'%{GLSL_LANG_VALIDATOR_BIN_DIR}glslangValidator -I"' ..SHADER_DIR .. '" -E %{file.relpath} > %{file.relpath}.preprocessed',

    //std::filesystem::path( shaderFileUrl ).parent_path().native()
    //const auto shaderBaseDir = std::filesystem::absolute( std::filesystem::path( "./src/shaders/" ) ).native();

#ifndef UNICODE 
    const auto shaderBaseDir = std::filesystem::absolute( std::filesystem::path( R"(.\src\shaders)" ) ).generic_string();
#else
    const auto shaderBaseDir = std::filesystem::absolute( std::filesystem::path( R"(.\src\shaders)" ) ).native();
#endif

    _tprintf( _T( "shaderBaseDir = %s\n" ), shaderBaseDir.c_str() );

    //const auto shaderBaseDirIncludeDirective = TinyProcessLib::Process::string_type{ _TEXT( "-I" ) } + shaderBaseDir;
    const auto shaderBaseDirIncludeDirective = TinyProcessLib::Process::string_type{ _TEXT( "-I\"" ) } + shaderBaseDir + _TEXT( "\"" );
    auto cmdLinePath = std::vector<TinyProcessLib::Process::string_type>{
        _TEXT( "cmd /C " ), // pipes work only when calling through cmd !!!
        //argv0Wide
        argv0Wide,
        //TinyProcessLib::Process::string_type{ _TEXT( "-I\"" ) } + shaderBaseDir + _TEXT( "\"" ),
        //TinyProcessLib::Process::string_type{ _TEXT( "-I" ) } + shaderBaseDir,
        shaderBaseDirIncludeDirective,
    #ifndef UNICODE 
        mShaderDefines,
    #else
        utf8Utils::utf8_decode( mShaderDefines ),
    #endif
        TinyProcessLib::Process::string_type{ _TEXT( "-E " ) },
    };

    printf( "---debug---\n" );
    for (const auto& procCmdLineEntry : cmdLinePath) {
        _tprintf( _T( "%s " ), procCmdLineEntry.c_str() );
    }
    printf( "\n---debug---\n" );

    printf( "found these shader files:\n" );
    for (const auto& shaderFileUrl : shaderFilenames) {
        //printf( "%s\n", shaderFileUrl.c_str() );

        auto thisCmdLinePath = cmdLinePath;
    #ifndef UNICODE 
        thisCmdLinePath.push_back( std::string( "\"" ) + std::filesystem::absolute( shaderFileUrl ).string() + "\"" );
        thisCmdLinePath.push_back( std::string( " > \"" ) + shaderFileUrl + ".preprocessed\"" );
    #else
        thisCmdLinePath.push_back( utf8Utils::utf8_decode( std::string( "\"" ) + std::filesystem::absolute( shaderFileUrl ).string() + "\"" ) );
        thisCmdLinePath.push_back( utf8Utils::utf8_decode( std::string( " > \"" ) + shaderFileUrl + ".preprocessed\"" ) );
    #endif
        //for (const auto& procCmdLineEntry : thisCmdLinePath) {
        //    _tprintf( _T( "%s " ), procCmdLineEntry.c_str() );
        //} printf( "\n" );

        //auto* pProcess = new TinyProcessLib::Process( 
        //    thisCmdLinePath, 
        //    //TinyProcessLib::Process::string_type( _TEXT( "dir" ) ),
        //    TinyProcessLib::Process::string_type( _TEXT( "" ) ),
        //    [](const char *bytes, size_t n) {
        //        std::cout << "Output from stdout: " << std::string(bytes, n);
        //    },
        //    []( const char* bytes, size_t n ) {
        //        std::cout << "Output from stderr: " << std::string( bytes, n );
        //    // Add a newline for prettier output on some platforms:
        //    if(bytes[n - 1] != '\n')
        //        std::cout << std::endl;
        //    }
        //);

        TinyProcessLib::Process::string_type cmdStr{ _T( "" ) }; 
        for (const auto& procCmdLineEntry : thisCmdLinePath) {
            cmdStr.append( procCmdLineEntry );
            cmdStr.append( _T( " " ) );
        }
        //cmdStr.append( utf8Utils::utf8_decode( std::string( " > \"" ) + std::filesystem::absolute( shaderFileUrl ).string() + ".preprocessed\"" ) );
        //_tprintf( _T( "cmdStr = \n    %s \n" ), cmdStr.c_str() );

    #if 0
        //auto* pProcess = new TinyProcessLib::Process( cmdStr );
        //auto* pProcess = new TinyProcessLib::Process(
        //    _T( R"(C:\FM-koop\DVR-fetch-test\DVR-t3dfw\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe -I"C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders" -E "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl" > "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl.preprocessed" )" ) );

        auto processTest = TinyProcessLib::Process(
            //_T( R"(C:\FM-koop\DVR-fetch-test\DVR-t3dfw\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe -I"C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders" -E "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl" > "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl.preprocessed" )" ) 
            //_T( R"(C:\FM-koop\DVR-fetch-test\DVR-t3dfw\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe -I"C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders" -E "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl" )" )
            _T( R"(cmd /C C:\FM-koop\DVR-fetch-test\DVR-t3dfw\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe -I"C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders" -E "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl" > "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl.test2" )" )
            , _T( R"(C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders)" )
            , nullptr,
            nullptr,
            false,
            TinyProcessLib::Config{ .inherit_file_descriptors = true }
        );
        int exitStatusProcessTest = processTest.get_exit_status();
        printf( "exitStatusProcessTest = %d\n", exitStatusProcessTest );
    #endif

        auto* pProcess = new TinyProcessLib::Process( cmdStr );
        
    #if 0
        bool openStdin = false;
        auto* pProcess = new TinyProcessLib::Process(
            cmdStr,
            TinyProcessLib::Process::string_type( _TEXT( "" ) ),
            [=]( const char* bytes, size_t n ) {
                //std::cout << "Output from stdout: " << std::string(bytes, n);
                //const auto fileUrl = std::string( "\"" ) + std::filesystem::absolute( shaderFileUrl ).string() + ".preprocessed\"";
                //const auto fileUrl = std::string( "\"" ) + std::filesystem::absolute( shaderFileUrl ).string() + ".test\"";
                //const auto fileUrl = std::filesystem::absolute( shaderFileUrl ).string() + ".test";
                const auto fileUrl = std::filesystem::absolute( shaderFileUrl ).string() + ".preprocessed";
                const auto fileContent = std::string( bytes, n );
                fileUtils::writeFile( fileUrl, fileContent );
            },
            []( const char* bytes, size_t n ) {
                std::cout << "Output from stderr: " << std::string( bytes, n );
            },
            openStdin//,
            //TinyProcessLib::Config{ .buffer_size = ((1 << 1024)<<1024) }
            );
    #endif

        int exitStatus = pProcess->get_exit_status();
        printf( "exit status = %d\n", exitStatus );

        delete pProcess;
        pProcess = nullptr;
        
    #if 0
        TinyProcessLib::Process::string_type cmdStr{ _T( "" ) }; 
        for (const auto& procCmdLineEntry : thisCmdLinePath) {
            cmdStr.append( procCmdLineEntry );
            cmdStr.append( _T( " " ) );
        }
        auto* pProcess = new TinyProcessLib::Process( 
            cmdStr, 
            TinyProcessLib::Process::string_type( _TEXT( "" ) ),
            [](const char *bytes, size_t n) {
                std::cout << "Output from stdout: " << std::string(bytes, n);
            },
            []( const char* bytes, size_t n ) {
                std::cout << "Output from stderr: " << std::string( bytes, n );
            // Add a newline for prettier output on some platforms:
            if(bytes[n - 1] != '\n')
                std::cout << std::endl;
            }

            );

        int exitStatus = pProcess->get_exit_status();
        printf( "exit status = %d\n", exitStatus );

        delete pProcess;
        pProcess = nullptr;
    #endif

        //TinyProcessLib::Process process1(
        //    //_T("cmd /C echo Hello World"), 
        //    _T( R"(C:\FM-koop\DVR-fetch-test\DVR-t3dfw\external\glslang-master-windows-x64-Release\bin\glslangValidator.exe -I"C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders" -E "C:\FM-koop\DVR-fetch-test\DVR-t3dfw\src\shaders\displayColors.frag.glsl")" ),
        //    _T(""), 
        //    [](const char *bytes, size_t n) {
        //        std::cout << "Output from stdout: " << std::string(bytes, n);
        //    });
        //auto exit_status = process1.get_exit_status();
        //std::cout << "Example 1 process returned: " << exit_status << " (" << (exit_status == 0 ? "success" : "failure") << ")" << std::endl;

        //break;
    }

    //int exitStatus;
    //if (mpColorPickerProcess == nullptr || mpColorPickerProcess->try_get_exit_status( exitStatus ) ) {
    //    printf( "appTF: LMB pressed and spinning up process!\n" );
    //    if (mpColorPickerProcess != nullptr) { 
    //        mpColorPickerProcess->kill(); 
    //        delete mpColorPickerProcess;
    //        mpColorPickerProcess = nullptr;
    //    }
    //    printf( "appTF: LMB pressed, look at the supplied args:\n" );
    //    int32_t i = 0;
    //    for ( const auto& cmdArg : mCmdLineProcess ) {
    //        //wprintf( "%d: %s\n", i, cmdArg.c_str() );
    //        std::wcout << i << " " << cmdArg << std::endl;
    //        i++;
    //    }
    //    mpColorPickerProcess = new TinyProcessLib::Process( mCmdLineProcess );
    //}

    return Status_t::OK();

}
