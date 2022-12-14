#ifndef _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

#include "gfxUtils.h"
// #include "gfxAPI/apiAbstractions.h"
#include "math/linAlg.h"

#include <string>
#include <vector>

namespace GfxAPI {
    struct ContextOpenGL;
    struct Texture;
}

namespace ArcBall {
    struct ArcBallControls;
}

namespace FileLoader {
    struct VolumeData;
}

struct ApplicationDVR : public iApplication {
    ApplicationDVR( 
        const GfxAPI::ContextOpenGL& contextOpenGL );
    virtual ~ApplicationDVR();

    Status_t load( const std::string& fileUrl );

    void setRotationPivotPos(   linAlg::vec3_t& rotPivotPosOS, 
                                linAlg::vec3_t& rotPivotPosWS, 
                                const int32_t& fbWidth, const int32_t& fbHeight, 
                                const float currMouseX, const float currMouseY );

    virtual Status_t run() override;

    void resetTransformations( ArcBall::ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle );

    private:
    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityTex3d;
    GfxAPI::Texture*                mpNormalTex3d;

    // Camera Params
    linAlg::mat3x4_t mModelMatrix3x4;
    linAlg::mat3x4_t mViewMatrix3x4;
    linAlg::mat4_t mModelViewMatrix;
    linAlg::mat4_t mProjMatrix;
    linAlg::mat4_t mMvpMatrix;

    // VAOs
    gfxUtils::bufferHandles_t mStlModelHandle{ .vaoHandle = static_cast<uint32_t>(-1) };
    gfxUtils::bufferHandles_t mScreenQuadHandle{ .vaoHandle = static_cast<uint32_t>(-1) };

    bool                            mGrabCursor;
};

#endif // _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
