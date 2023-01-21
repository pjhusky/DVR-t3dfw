#ifndef _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

#include "gfxUtils.h"
#include "math/linAlg.h"

#include "external/tiny-process-library/process.hpp"
#include "sharedMemIPC.h"

#include <string>
#include <vector>

namespace GfxAPI {
    struct ContextOpenGL;
    struct Texture;
}

namespace FileLoader {
    struct VolumeData;
}

struct ApplicationTransferFunction : public iApplication {
    ApplicationTransferFunction( 
        const GfxAPI::ContextOpenGL& contextOpenGL );
    virtual ~ApplicationTransferFunction();

    Status_t load( const std::string& fileUrl );

    void setRotationPivotPos(   linAlg::vec3_t& rotPivotPosOS, 
                                linAlg::vec3_t& rotPivotPosWS, 
                                const int32_t& fbWidth, const int32_t& fbHeight, 
                                const float currMouseX, const float currMouseY );

    virtual Status_t run() override;

    void setCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLinePath ) { mCmdLinePath = cmdLinePath; }

    private:
    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityHistogramTex2d;

    // VAOs
    gfxUtils::bufferHandles_t mScreenQuadHandle{ .vaoHandle = static_cast<uint32_t>(-1) };

    std::vector<TinyProcessLib::Process::string_type>    mCmdLinePath;
    TinyProcessLib::Process* mpProcess;

    SharedMemIPC                    mSharedMem;

    bool                            mGrabCursor;
};

#endif // _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb
