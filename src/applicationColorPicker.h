#ifndef _ApplicationColorPicker_H_6635ff59_825e_41bd_9358_85cad169f5eb
#define _ApplicationColorPicker_H_6635ff59_825e_41bd_9358_85cad169f5eb

#include "applicationInterface/iApplication.h"

#include <string>
#include <vector>

#include "gfxUtils.h"

namespace GfxAPI {
    struct ContextOpenGL;
    struct Texture;
    struct Shader;
}

struct ApplicationColorPicker : public iApplication {
    ApplicationColorPicker( 
        const GfxAPI::ContextOpenGL& contextOpenGL );

    virtual Status_t run() override;
    Status_t drawGfx();

    private:
    
    const GfxAPI::ContextOpenGL&    mContextOpenGL;

    int32_t mFbWidth, mFbHeight;
    int32_t mPrevFbWidth;
    int32_t mPrevFbHeight;
    gfxUtils::bufferHandles_t mScreenFill_VAO{ .vaoHandle = static_cast<uint32_t>(-1) };
    GfxAPI::Shader* mpShader;
};

#endif // _ApplicationColorPicker_H_6635ff59_825e_41bd_9358_85cad169f5eb
