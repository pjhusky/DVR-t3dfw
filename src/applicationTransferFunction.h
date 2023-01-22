#ifndef _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

#include "gfxUtils.h"
#include "math/linAlg.h"

#include "external/tiny-process-library/process.hpp"
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
        const GfxAPI::ContextOpenGL& contextOpenGL );
    virtual ~ApplicationTransferFunction();

    Status_t load( const std::string& fileUrl );

    virtual Status_t run() override;

    void setCommandLinePath( const std::vector<TinyProcessLib::Process::string_type>& cmdLine ) { mCmdLineColorPickerProcess = cmdLine; }

private:
    const GfxAPI::ContextOpenGL&    mContextOpenGL;
    std::string                     mDataFileUrl;
    FileLoader::VolumeData*         mpData;
    GfxAPI::Texture*                mpDensityTransparencyTex2d;
    GfxAPI::Texture*                mpDensityColorsTex2d;
    GfxAPI::Texture*                mpDensityHistogramTex2d;

    //struct colorElement_t {
    //    uint32_t       index;
    //    linAlg::vec3_t color;
    //};
    std::map< uint32_t, linAlg::vec3_t >     mDensityColors;

    // VAOs
    gfxUtils::bufferHandles_t mScreenQuadHandle{ .vaoHandle = static_cast<uint32_t>(-1) };

    std::vector<TinyProcessLib::Process::string_type>    mCmdLineColorPickerProcess;
    TinyProcessLib::Process* mpColorPickerProcess;

    SharedMemIPC                    mSharedMem;

    bool                            mGrabCursor;
};

#endif // _ApplicationTransferFunction_H_6635ff59_825e_41bd_9358_85cad169f5eb
