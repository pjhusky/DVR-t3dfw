#ifndef _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

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

    virtual Status_t run() override;

    void resetTransformations( ArcBall::ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle );

    private:
    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityTex3d;
    GfxAPI::Texture*                mpNormalTex3d;
    bool                            mGrabCursor;
};

#endif // _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
