#ifndef _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"
#include "applicationDVR_common.h"

#include "gfxUtils.h"
#include "math/linAlg.h"

#include "tiny-process-library/process.hpp"
#include "sharedMemIPC.h"

#include <string>
#include <vector>
#include <map>


namespace GfxAPI {
    struct ContextOpenGL;
    struct Texture;
}

namespace FileLoader {
    struct VolumeData;
}

struct ApplicationTransferFunction : public iApplication {
    ApplicationTransferFunction( 
        const GfxAPI::ContextOpenGL& contextOpenGL,
        const bool checkWatchdog = true );
    virtual ~ApplicationTransferFunction();

    Status_t load( const std::string& fileUrl );
    Status_t save( const std::string& fileUrl );
    void reset();

    virtual Status_t run() override;

    //void setTFCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLine ) { mCmdLineColorPickerProcess = cmdLine; }

private:

    void colorKeysToTex2d();
    void densityHistogramToTex2d();
    void densityTransparenciesToTex2d();

    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityTransparenciesTex2d;
    GfxAPI::Texture*                mpDensityColorsTex2d;
    GfxAPI::Texture*                mpDensityColorDotTex2d;
    GfxAPI::Texture*                mpDensityHistogramTex2d;
    GfxAPI::Texture*                mpIsovalMarkerTex2d;

    std::array<uint8_t, ApplicationDVR_common::numDensityBuckets * 4>   mInterpolatedDataCPU;

    std::vector< uint32_t >         mHistogramBuckets;

    std::vector<uint8_t>            mTransparencyPaintHeightsCPU;
    linAlg::vec4_t                  mScaleAndOffset_Transparencies;
    linAlg::vec4_t                  mScaleAndOffset_Colors;
    linAlg::vec4_t                  mScaleAndOffset_Isovalues;
    

    //struct colorElement_t {
    //    uint32_t       index;
    //    linAlg::vec3_t color;
    //};
    using index_t = uint32_t;
    using color_t = linAlg::vec3_t;
    std::map< index_t, color_t >    mDensityColors;
    uint32_t                        mNumHistogramBuckets;

    // VAOs
    gfxUtils::bufferHandles_t mScreenQuadHandle{ .vaoHandle = static_cast<uint32_t>(-1) };

    //std::vector<TinyProcessLib::Process::string_type>   mCmdLineColorPickerProcess;
    //TinyProcessLib::Process*                            mpColorPickerProcess;

    SharedMemIPC                    mSharedMem;

    bool                            mGrabCursor;
    bool                            mCheckWatchdog;
};

#endif // _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb
