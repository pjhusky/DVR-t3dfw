#ifndef _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

#include "gfxUtils.h"
// #include "gfxAPI/apiAbstractions.h"
#include "math/linAlg.h"

#include "tiny-process-library/process.hpp"
#include "sharedMemIPC.h"

#include <string>
#include <vector>

namespace GfxAPI {
    struct ContextOpenGL;
    struct Texture;
    struct Rbo;
    struct Fbo;
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

    void setRotationPivotPos(   linAlg::vec3_t& rotPivotPosOS, 
                                linAlg::vec3_t& rotPivotPosWS, 
                                const int32_t& fbWidth, const int32_t& fbHeight, 
                                const float currMouseX, const float currMouseY );

    virtual Status_t run() override;
    
    void handleScreenResize( 
        GLFWwindow* const pWindow,
        linAlg::mat4_t& projMatrix,
        int32_t& prevFbWidth,
        int32_t& prevFbHeight,
        int32_t& fbWidth,
        int32_t& fbHeight );

    void tryStartTransferFunctionApp();

    void resetTransformations( ArcBall::ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle );

    void setCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLinePath ) { mCmdLinePath = cmdLinePath; }

    private:
    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityTex3d;
    GfxAPI::Texture*                mpGradientTex3d;
    GfxAPI::Texture*                mpDensityColorsTex2d;

#if 0
    GfxAPI::Texture*                mpGuiTex;
    GfxAPI::Fbo*                    mpGuiFbo;
#endif

    GfxAPI::Texture*                mpVol_RT_Tex;
    GfxAPI::Rbo*                    mpVol_RT_Rbo;
    GfxAPI::Fbo*                    mpVol_RT_Fbo;

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

    std::vector<TinyProcessLib::Process::string_type>    mCmdLinePath;
    TinyProcessLib::Process* mpProcess;

    SharedMemIPC                    mSharedMem;

    std::thread*                    mpWatchdogThread;

    bool                            mGrabCursor;
};

#endif // _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
