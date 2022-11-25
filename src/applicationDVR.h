#ifndef _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"
#include "fileLoaders/volumeData.h"

#include <string>
#include <vector>

struct ContextOpenGL;
struct Texture;
struct ArcBallControls;

struct ApplicationDVR : public iApplication {
    ApplicationDVR( 
        const ContextOpenGL& contextOpenGL );
    virtual ~ApplicationDVR();

    Status_t load( const std::string& fileUrl );

    virtual Status_t run() override;

    void resetTransformations( ArcBallControls& arcBallControl, float& camTiltRadAngle, float& targetCamTiltRadAngle );

    private:
    const ContextOpenGL&    mContextOpenGL;
    std::string             mDataFileUrl;
    VolumeData*             mpData;
    Texture*                mpDensityTex3d;
    Texture*                mpNormalTex3d;
};

#endif // _ApplicationDVR_H_6635ff59_825e_41bd_9358_85cad169f5eb
