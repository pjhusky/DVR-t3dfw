#ifndef _SCREEN_LABEL_MGR_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
#define _SCREEN_LABEL_MGR_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0

#include "singleton.h"

#include "dataLabel.h"
#include "stbFont.h"
#include "ttfMeshFont.h"

#include "gfxUtils.h"

#include "gfxAPI/shader.h"
#include "gfxAPI/vbo.h"
#include "gfxAPI/bufferTexture.h"

#include <vector>
#include <stdint.h>

struct dataLabelMgr final : public Singleton<dataLabelMgr> {
    friend class Singleton<dataLabelMgr>;

    void addLabel( const dataLabel& dataLabel );
    void removeAllLabels();
    void bakeAddedLabels();
    void drawScreen( const int32_t fbW, const int32_t fbH, uint64_t frameNum );

private:
    //dataLabelMgr( token ); // could be public this way...?

    dataLabelMgr();
    ~dataLabelMgr();

    std::vector< dataLabel >    mScreenLabels;
    stbFont                     mStbFont; // remove later
    ttfMeshFont                 mTtfMeshFont;

    GfxAPI::Shader              mFullscreenTriShader;
    gfxUtils::bufferHandles_t   mScreenTriHandle{ .vaoHandle = static_cast<uint32_t>(-1) };

    GfxAPI::Shader              mSdfShader;
    GfxAPI::Vbo*                mpSdfRoundRectsVbo;
    GfxAPI::Vbo*                mpSdfArrowsVbo;

    GfxAPI::BufferTexture       mSdfRoundRectsBT;
    GfxAPI::BufferTexture       mSdfArrowsBT;

};

#endif // _SCREEN_LABEL_MGR_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
