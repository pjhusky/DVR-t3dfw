#ifndef _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

#include "gfxUtils.h"
// #include "gfxAPI/apiAbstractions.h"
#include "math/linAlg.h"

#include "tiny-process-library/process.hpp"
#include "sharedMemIPC.h"
#include "stbFont.h"

#include "GUI/DVR_GUI.h"


#include <string>
#include <vector>

namespace GfxAPI {
    struct ContextOpenGL;
    struct Texture;
    struct Rbo;
    struct Fbo;
    struct Ubo;
}

namespace ArcBall {
    struct ArcBallControls;
}

namespace FileLoader {
    struct VolumeData;
}

struct GLFWwindow;

struct ApplicationDVR : public iApplication {
    ApplicationDVR( 
        const GfxAPI::ContextOpenGL& contextOpenGL );
    virtual ~ApplicationDVR();

    Status_t load( const std::string& fileUrl, const int32_t gradientMode );
    Status_t loadLabels( const std::string& fileUrl );

    void setRotationPivotPos(   linAlg::vec3_t& rotPivotPosOS, 
                                linAlg::vec3_t& rotPivotPosWS, 
                                const int32_t& fbWidth, const int32_t& fbHeight, 
                                const float currMouseX, const float currMouseY );

    void LoadDVR_Shaders(   const DVR_GUI::eVisAlgo visAlgo, 
                            const DVR_GUI::eDebugVisMode debugVisMode, 
                            const bool useEmptySpaceSkipping, 
                            GfxAPI::Shader& meshShader, 
                            GfxAPI::Shader& volShader );

    virtual Status_t run() override;
    
    void handleScreenResize( 
        GLFWwindow* const pWindow,
        linAlg::mat4_t& projMatrix,
        int32_t& prevFbWidth,
        int32_t& prevFbHeight,
        int32_t& fbWidth,
        int32_t& fbHeight );

    void tryStartTransferFunctionApp();
    void tryStartShaderCompilerApp();

    void resetTransformations( ArcBall::ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle );
    void resetTF();

    void setTFCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLinePath ) { mTFCmdLinePath = cmdLinePath; }
    void setSCCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLinePath ) { mSCCmdLinePath = cmdLinePath; }

    private:
        void fixupShaders( GfxAPI::Shader& meshShader, GfxAPI::Shader& volShader );

    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityTex3d;
    GfxAPI::Texture*                mpDensityLoResTex3d;
    GfxAPI::Texture*                mpEmptySpaceTex2d;
    GfxAPI::Texture*                mpGradientTex3d;
    GfxAPI::Texture*                mpDensityColorsTex2d;

#if 0
    GfxAPI::Texture*                mpGuiTex;
    GfxAPI::Fbo*                    mpGuiFbo;
#endif

    GfxAPI::Texture*                mpVol_RT_Tex;
    GfxAPI::Rbo*                    mpVol_RT_Rbo;
    GfxAPI::Fbo*                    mpVol_RT_Fbo;

    GfxAPI::Ubo*                    mpLight_Ubo;

    std::vector< std::array<uint16_t,2> > mVolLoResData;
    std::vector<uint8_t> mEmptySpaceTableData;

    linAlg::i32vec3_t mVolLoResEmptySpaceSkipDim;

    // Camera Params
    linAlg::mat3x4_t mModelMatrix3x4;
    linAlg::mat3x4_t mViewMatrix3x4;
    linAlg::mat4_t mModelViewMatrix;
    linAlg::mat4_t mProjMatrix;
    linAlg::mat4_t mMvpMatrix;

    // VAOs
    gfxUtils::bufferHandles_t mStlModelHandle{ .vaoHandle = static_cast<uint32_t>(-1) };
    gfxUtils::bufferHandles_t mScreenQuadHandle{ .vaoHandle = static_cast<uint32_t>(-1) };
    gfxUtils::bufferHandles_t mScreenTriHandle{ .vaoHandle = static_cast<uint32_t>(-1) };

    std::vector<TinyProcessLib::Process::string_type>    mTFCmdLinePath;
    TinyProcessLib::Process*                             mpTFProcess;

    std::vector<TinyProcessLib::Process::string_type>    mSCCmdLinePath;
    TinyProcessLib::Process*                             mpSCProcess;

    SharedMemIPC                    mSharedMem;

    stbFont                         mStbFont;

    std::thread*                    mpWatchdogThread;

    bool                            mGrabCursor;
};

#endif // _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
