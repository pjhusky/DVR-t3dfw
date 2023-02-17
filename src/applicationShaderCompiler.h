#ifndef _APPLICATION_SHADER_COMPILER_H_3AFFE9E6_2279_415B_A70D_816A412D6CA1
#define _APPLICATION_SHADER_COMPILER_H_3AFFE9E6_2279_415B_A70D_816A412D6CA1

#include "applicationInterface/iApplication.h"

#include "tiny-process-library/process.hpp"

#include <filesystem>
//#include <map>
#include <string>

struct ApplicationShaderCompiler : public iApplication {
    ApplicationShaderCompiler( const std::string& shaderDefines /*const std::filesystem::path& baseShaderDir*/ );

    virtual Status_t run() override;
    //void setSCCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLine ) { mSCCmdLineProcess = cmdLine; }

    //void clearDefines() { mDefineMatrix.clear(); }
    //void addDefine( const std::string& token, const std::string& value = "" ) { mDefineMatrix.insert( {token, value} ); }

private:
    //std::map< std::string, std::string >                mDefineMatrix; // main DVR app will have to communicate changes as these through shared mem!!!
    //std::vector<TinyProcessLib::Process::string_type>   mSCCmdLineProcess;
    TinyProcessLib::Process*                            mpProcess;

    std::string mShaderDefines;
};

#endif // _APPLICATION_SHADER_COMPILER_H_3AFFE9E6_2279_415B_A70D_816A412D6CA1

